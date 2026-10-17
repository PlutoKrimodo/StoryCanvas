"""依赖注入：Provider 与 Service 工厂、内部鉴权。

`LLM_PROVIDER` / `IMAGE_PROVIDER` 开关在此收敛为「选择具体实现」的唯一入口，
其余层只依赖抽象基类（对照 docs/16 §4.2 决策 7）。
"""

from __future__ import annotations

from functools import lru_cache
from typing import Dict, Optional, Type
import asyncio

from fastapi import Header, HTTPException

from ...models.schemas import ImageCredential, ImageGenerateRequest, LLMCredential
from ...providers.image.base import ImageProvider
from ...providers.image.doubao_provider import DoubaoProvider
from ...providers.image.mock_provider import MockImageProvider
from ...providers.image.wanx_provider import WanxProvider
from ...providers.llm.base import LLMProvider
from ...providers.llm.deepseek_provider import DeepSeekProvider
from ...providers.llm.mock_provider import MockLLMProvider
from ...providers.llm.openai_compatible import OpenAICompatibleProvider
from ...services.generation_service import GenerationService
from ...services.storyboard_service import StoryboardService
from ...utils.exceptions import ImageProviderException, LLMProviderException
from ...services.image_generator import ImageGeneratorService
from ...services.prompt_optimizer import PromptOptimizerService
from ...services.text_analyzer import TextAnalyzerService
from ...utils.config import settings
from ...utils.url_safety import UnsafeUrlError, validate_public_https_url

_LLM_PROVIDERS: Dict[str, Type[LLMProvider]] = {
    "deepseek": DeepSeekProvider,
    "mock": MockLLMProvider,
}

_IMAGE_PROVIDERS: Dict[str, Type[ImageProvider]] = {
    "doubao": DoubaoProvider,
    "wanx": WanxProvider,
    "mock": MockImageProvider,
}


@lru_cache(maxsize=None)
def get_llm_provider() -> LLMProvider:
    """按 `LLM_PROVIDER` 选择 LLM 实现（未知值回退 mock，保证服务可用）。"""
    key = settings.LLM_PROVIDER.strip().lower()
    return _LLM_PROVIDERS.get(key, MockLLMProvider)()


@lru_cache(maxsize=None)
def get_image_provider() -> ImageProvider:
    """按 `IMAGE_PROVIDER` 选择图像实现（未知值回退 mock）。"""
    key = settings.IMAGE_PROVIDER.strip().lower()
    return _IMAGE_PROVIDERS.get(key, MockImageProvider)()


def get_text_analyzer() -> TextAnalyzerService:
    return TextAnalyzerService(get_llm_provider())


def get_prompt_optimizer() -> PromptOptimizerService:
    return PromptOptimizerService()


def get_image_generator() -> ImageGeneratorService:
    return ImageGeneratorService(get_image_provider())


def get_generation_service() -> GenerationService:
    return GenerationService(
        analyzer=get_text_analyzer(),
        optimizer=get_prompt_optimizer(),
        image_generator=get_image_generator(),
    )


# ==================== BYOK：按请求构造 Provider ====================

def build_llm_provider(credential: LLMCredential) -> LLMProvider:
    """按用户凭据构造 LLM Provider。

    注意：**绝不能**走 `lru_cache`，否则第一个用户的 Key 会被后续请求复用。
    """
    if credential.provider == "mock":
        # 仅测试便利：BYOK 接口不会下发 mock，但保留可预测行为
        return MockLLMProvider()
    if credential.provider not in ("deepseek", "openai", "dashscope", "zhipu", "custom"):
        raise LLMProviderException(f"不支持的文本模型厂商：{credential.provider}", 422)
    return OpenAICompatibleProvider(
        api_key=credential.api_key,
        api_base=credential.api_base,
        model=credential.model,
        provider=credential.provider,
    )


def build_image_provider(credential: ImageCredential) -> ImageProvider:
    """按用户凭据构造图像 Provider；不做跨厂商兜底。"""
    if credential.provider == "doubao":
        return DoubaoProvider(
            api_key=credential.api_key,
            model=credential.model,
            api_base=credential.api_base,
        )
    if credential.provider == "wanx":
        return WanxProvider(
            api_key=credential.api_key,
            model=credential.model,
            api_base=credential.api_base,
        )
    raise ImageProviderException(f"不支持的图像厂商：{credential.provider}", 422)


def build_storyboard_service(credential: Optional[LLMCredential]) -> StoryboardService:
    """分镜与出图使用同一套 BYOK 口径：有用户凭据就按请求构造 Provider。"""
    provider = build_llm_provider(credential) if credential is not None else get_llm_provider()
    return StoryboardService(provider)


async def ensure_credential_base_safe(request) -> None:
    """用户自定义 Base URL 必须先通过 SSRF 校验（DNS 解析放到线程池）。

    分镜（仅 LLM 凭据）与图像生成（LLM + 图像凭据）共用同一套规则。
    """
    bases = []
    llm_credential = getattr(request, "llm_credential", None)
    image_credential = getattr(request, "image_credential", None)
    if llm_credential is not None and llm_credential.api_base:
        bases.append(llm_credential.api_base)
    if image_credential is not None and image_credential.api_base:
        bases.append(image_credential.api_base)

    for base in bases:
        try:
            await asyncio.to_thread(validate_public_https_url, base)
        except UnsafeUrlError as exc:
            raise HTTPException(
                status_code=422, detail=f"自定义 Base URL 不安全：{exc}"
            ) from exc


def build_generation_service(request: ImageGenerateRequest) -> GenerationService:
    """按请求中是否携带凭据，决定使用用户 Key 还是系统默认 Provider。"""
    llm_provider: LLMProvider = (
        build_llm_provider(request.llm_credential)
        if request.llm_credential is not None
        else get_llm_provider()
    )
    image_provider: ImageProvider = (
        build_image_provider(request.image_credential)
        if request.image_credential is not None
        else get_image_provider()
    )
    return GenerationService(
        analyzer=TextAnalyzerService(llm_provider),
        optimizer=PromptOptimizerService(),
        image_generator=ImageGeneratorService(image_provider),
    )


async def verify_internal_key(
    x_internal_api_key: str = Header(default="", alias="X-Internal-Api-Key")
) -> None:
    """内网调用鉴权。

    约定：`INTERNAL_API_KEY` 未配置时不启用校验（便于本地开发 / 单测）；
    一旦配置，则所有业务接口必须携带一致的 `X-Internal-Api-Key`。
    """
    expected = settings.INTERNAL_API_KEY
    if expected and x_internal_api_key != expected:
        raise HTTPException(status_code=401, detail="内部鉴权失败")


def reset_provider_cache() -> None:
    """清空 Provider 缓存（测试中切换开关后调用）。"""
    get_llm_provider.cache_clear()
    get_image_provider.cache_clear()
