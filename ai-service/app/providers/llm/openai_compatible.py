"""通用 OpenAI 兼容 LLM Provider（BYOK 使用）。

DeepSeek / OpenAI / 通义千问（兼容模式）/ 智谱 GLM 均实现
`POST {api_base}/chat/completions` + Bearer 鉴权，因此按请求传入
api_key / api_base / model 即可，无需为每家厂商复制一份实现。

安全约定：
  - 本 Provider 只能由 BYOK 请求显式构造，**不进入** `lru_cache`；
  - 异常信息只包含状态码与可读原因，绝不拼接请求头或响应体。
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional

from ...models.schemas import ArtStyle  # noqa: F401  (保留类型引用语义)
from ...utils.config import settings
from ...utils.exceptions import LLMProviderException
from ...utils.http_client import describe_http_error, post_json
from .base import LLMProvider, Message


@dataclass(frozen=True)
class LLMPreset:
    label: str
    api_base: str
    model: str


# 与 C++ 后端 utils/AiCredentialRules.cpp 的目录保持一致；此处仅作兜底，
# 正常请求会由后端补全 api_base / model。
_PRESETS: Dict[str, LLMPreset] = {
    "deepseek": LLMPreset("DeepSeek", "https://api.deepseek.com", "deepseek-chat"),
    "openai": LLMPreset("OpenAI", "https://api.openai.com/v1", "gpt-4o-mini"),
    "dashscope": LLMPreset(
        "通义千问", "https://dashscope.aliyuncs.com/compatible-mode/v1", "qwen-plus"
    ),
    "zhipu": LLMPreset("智谱 GLM", "https://open.bigmodel.cn/api/paas/v4", "glm-4-flash"),
    "custom": LLMPreset("自定义服务", "", ""),
}


class OpenAICompatibleProvider(LLMProvider):
    """按请求凭据构造的 OpenAI 兼容实现。"""

    def __init__(
        self,
        api_key: str,
        api_base: Optional[str] = None,
        model: Optional[str] = None,
        provider: str = "custom",
    ) -> None:
        super().__init__()
        preset = _PRESETS.get(provider, _PRESETS["custom"])
        self.provider = provider
        self.api_key = api_key or ""
        self.api_base = (api_base or preset.api_base).rstrip("/")
        self.model = model or preset.model
        self.label = preset.label

    @property
    def name(self) -> str:
        return f"{self.provider}:{self.model}"

    async def generate(
        self,
        messages: List[Message],
        *,
        temperature: float = 0.7,
        max_tokens: int = 1000,
        json_mode: bool = False,
    ) -> str:
        if not self.api_key:
            raise LLMProviderException(f"{self.label} API Key 未配置", 503)
        if not self.api_base or not self.model:
            raise LLMProviderException(f"{self.label} 的 Base URL 或模型未配置", 422)

        payload: Dict[str, object] = {
            "model": self.model,
            "messages": messages,
            "temperature": temperature,
            "max_tokens": max_tokens,
            "stream": False,
        }
        if json_mode:
            payload["response_format"] = {"type": "json_object"}

        try:
            data = await post_json(
                f"{self.api_base}/chat/completions",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json",
                },
                payload=payload,
                timeout=settings.LLM_TIMEOUT,
            )
        except Exception as exc:  # httpx / 网络异常统一包装
            status, reason = describe_http_error(exc)
            # 401/402/403/429 快速失败，避免上层重试造成重复计费
            code = status if status in (401, 402, 403, 429) else 502
            self.logger.error("%s 调用失败: %s", self.label, reason)
            raise LLMProviderException(f"{self.label} 调用失败：{reason}", code) from exc

        try:
            return data["choices"][0]["message"]["content"]
        except (KeyError, IndexError, TypeError) as exc:
            raise LLMProviderException(f"{self.label} 响应格式异常", 502) from exc

    async def health_check(self) -> bool:
        return bool(self.api_key and self.api_base and self.model)
