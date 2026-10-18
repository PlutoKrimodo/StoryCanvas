"""豆包·Seedream（火山方舟）图像生成 Provider（真实实现，演示主用）。

接口：`POST {ARK_API_BASE}/images/generations`（OpenAI Images 风格，同步返回）。
"""

from __future__ import annotations

from typing import Dict, Optional

import httpx

from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings
from ...utils.exceptions import ImageProviderException
from ...utils.http_client import describe_http_error, post_json
from .base import ImageProvider

# 只有 Seedream 3.0 系列支持 seed；4.0/4.5/5.0 均不支持，发送会被判为非法参数。
_SEED_MODEL_HINTS = ("seedream-3-0",)


class DoubaoProvider(ImageProvider):
    """火山方舟 · 豆包 Seedream 实现。"""

    def __init__(
        self,
        api_key: Optional[str] = None,
        model: Optional[str] = None,
        api_base: Optional[str] = None,
    ) -> None:
        super().__init__()
        self.api_key = api_key if api_key is not None else settings.ARK_API_KEY
        self.api_base = (api_base or settings.ARK_API_BASE).rstrip("/")
        self.model = model or settings.ARK_IMAGE_MODEL

    @property
    def name(self) -> str:
        return f"doubao:{self.model}"

    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        if not self.api_key:
            raise ImageProviderException("ARK_API_KEY 未配置，无法调用豆包 Seedream", 503)

        prompt = params.prompt
        if params.negative_prompt:
            # ARK 不解析 --no 语法；负向约束用自然语言写入 prompt
            prompt = f"{prompt}\n\nAvoid: {params.negative_prompt}"

        payload: Dict[str, object] = {
            "model": self.model,
            "prompt": prompt,
            "size": f"{params.width}x{params.height}",
            "response_format": "url",
            "watermark": False,
        }
        # Seedream 4.0/4.5/5.0 不支持 seed；仅 3.0 系列保留可复现能力
        if params.seed is not None and any(
            hint in self.model for hint in _SEED_MODEL_HINTS
        ):
            payload["seed"] = params.seed

        try:
            data = await post_json(
                f"{self.api_base}/images/generations",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json",
                },
                payload=payload,
                timeout=settings.IMAGE_TIMEOUT,
            )
        except Exception as exc:
            status, reason = describe_http_error(exc)
            # 401/402/403/429 快速失败，避免重试导致重复计费
            code = status if status in (401, 402, 403, 429) else 502
            if isinstance(exc, httpx.HTTPStatusError):
                # 只记录厂商错误码与消息，便于定位非法字段；不输出 Key / prompt
                detail: Dict[str, object] = {}
                try:
                    body = exc.response.json()
                    error = body.get("error") if isinstance(body, dict) else None
                    if isinstance(error, dict):
                        detail = error
                except Exception:
                    detail = {}
                detail_code = str(detail.get("code", ""))
                detail_message = str(detail.get("message", ""))
                self.logger.error(
                    "豆包 Seedream 调用失败: %s | code=%s message=%s",
                    reason,
                    detail_code,
                    detail_message,
                )
                # 版权 / 内容安全策略是确定性失败，快速失败并给出可读原因，避免无意义重试
                if (
                    "SensitiveContentDetected" in detail_code
                    or "PolicyViolation" in detail_code
                    or "copyright" in detail_message.lower()
                ):
                    raise ImageProviderException(
                        "图像涉及版权或内容安全限制，请使用原创角色与场景后重试",
                        403,
                    ) from exc
            else:
                self.logger.error("豆包 Seedream 调用失败: %s", reason)
            raise ImageProviderException(f"豆包 Seedream 调用失败：{reason}", code) from exc

        return self._to_result(data, params)

    @staticmethod
    def _to_result(data: Dict[str, object], params: ImageGenerationParams) -> ImageGenerationResult:
        try:
            items = data["data"]  # type: ignore[index]
            first = items[0]  # type: ignore[index]
        except (KeyError, IndexError, TypeError) as exc:
            raise ImageProviderException(f"豆包 Seedream 响应格式异常: {data}", 502) from exc

        url = first.get("url") if isinstance(first, dict) else None
        b64 = first.get("b64_json") if isinstance(first, dict) else None
        if not url and not b64:
            raise ImageProviderException(f"豆包 Seedream 未返回图片: {data}", 502)

        return ImageGenerationResult(
            image_url=url or "",
            width=params.width,
            height=params.height,
            seed=params.seed,
            finish_reason="success",
        )

    async def health_check(self) -> bool:
        # 火山方舟无轻量探测接口，这里只做密钥存在性判断，避免产生额外计费
        return bool(self.api_key)
