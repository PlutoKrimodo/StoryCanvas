"""DeepSeek LLM Provider（真实实现，演示默认）。

DeepSeek 提供 OpenAI 兼容的 `/chat/completions` 接口，
因此这里直接用 httpx 调 HTTP API，无需引入 openai SDK（见 v2.2 变更）。
"""

from __future__ import annotations

from typing import Dict, List, Optional

from ...models.schemas import ArtStyle  # noqa: F401  (保留类型引用语义)
from ...utils.config import settings
from ...utils.exceptions import LLMProviderException
from ...utils.http_client import post_json
from .base import LLMProvider, Message


class DeepSeekProvider(LLMProvider):
    """DeepSeek Chat 实现。"""

    def __init__(self, api_key: Optional[str] = None, model: Optional[str] = None) -> None:
        super().__init__()
        self.api_key = api_key if api_key is not None else settings.DEEPSEEK_API_KEY
        self.api_base = settings.DEEPSEEK_API_BASE.rstrip("/")
        self.model = model or settings.DEEPSEEK_MODEL

    @property
    def name(self) -> str:
        return f"deepseek:{self.model}"

    async def generate(
        self,
        messages: List[Message],
        *,
        temperature: float = 0.7,
        max_tokens: int = 1000,
        json_mode: bool = False,
    ) -> str:
        if not self.api_key:
            raise LLMProviderException("DEEPSEEK_API_KEY 未配置，无法调用 DeepSeek", 503)

        payload: Dict[str, object] = {
            "model": self.model,
            "messages": messages,
            "temperature": temperature,
            "max_tokens": max_tokens,
            "stream": False,
        }
        if json_mode:
            # DeepSeek 要求 JSON 模式时 prompt 中必须出现 "json" 字样（见官方文档）
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
            self.logger.error("DeepSeek 调用失败: %s", exc)
            raise LLMProviderException(f"DeepSeek 调用失败: {exc}", 502) from exc

        try:
            return data["choices"][0]["message"]["content"]
        except (KeyError, IndexError, TypeError) as exc:
            raise LLMProviderException(f"DeepSeek 响应格式异常: {data}", 502) from exc

    async def health_check(self) -> bool:
        if not self.api_key:
            return False
        try:
            await post_json(
                f"{self.api_base}/chat/completions",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json",
                },
                payload={
                    "model": self.model,
                    "messages": [{"role": "user", "content": "ping"}],
                    "max_tokens": 1,
                },
                timeout=settings.LLM_TIMEOUT,
            )
            return True
        except Exception:
            return False
