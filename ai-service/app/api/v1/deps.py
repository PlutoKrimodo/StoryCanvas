"""依赖注入：Provider 与 Service 工厂、内部鉴权。

`LLM_PROVIDER` / `IMAGE_PROVIDER` 开关在此收敛为「选择具体实现」的唯一入口，
其余层只依赖抽象基类（对照 docs/16 §4.2 决策 7）。
"""

from __future__ import annotations

from functools import lru_cache
from typing import Dict, Type

from fastapi import Header, HTTPException

from ...providers.image.base import ImageProvider
from ...providers.image.doubao_provider import DoubaoProvider
from ...providers.image.mock_provider import MockImageProvider
from ...providers.image.wanx_provider import WanxProvider
from ...providers.llm.base import LLMProvider
from ...providers.llm.deepseek_provider import DeepSeekProvider
from ...providers.llm.mock_provider import MockLLMProvider
from ...services.generation_service import GenerationService
from ...services.image_generator import ImageGeneratorService
from ...services.prompt_optimizer import PromptOptimizerService
from ...services.text_analyzer import TextAnalyzerService
from ...utils.config import settings

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
