"""豆包·Seedream（火山方舟）图像生成 Provider（真实实现，演示主用）。

接口：`POST {ARK_API_BASE}/images/generations`（OpenAI Images 风格，同步返回）。
"""

from __future__ import annotations

from typing import Dict, Optional

from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings
from ...utils.exceptions import ImageProviderException
from ...utils.http_client import post_json
from .base import ImageProvider


class DoubaoProvider(ImageProvider):
    """火山方舟 · 豆包 Seedream 实现。"""

    def __init__(self, api_key: Optional[str] = None, model: Optional[str] = None) -> None:
        super().__init__()
        self.api_key = api_key if api_key is not None else settings.ARK_API_KEY
        self.api_base = settings.ARK_API_BASE.rstrip("/")
        self.model = model or settings.ARK_IMAGE_MODEL

    @property
    def name(self) -> str:
        return f"doubao:{self.model}"

    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        if not self.api_key:
            raise ImageProviderException("ARK_API_KEY 未配置，无法调用豆包 Seedream", 503)

        payload: Dict[str, object] = {
            "model": self.model,
            "prompt": params.prompt,
            "size": f"{params.width}x{params.height}",
            "n": 1,
            "response_format": "url",
            "watermark": False,
        }
        if params.negative_prompt:
            # Seedream 通过 prompt 内联负向提示时兼容性最好
            payload["prompt"] = f"{params.prompt}\n\n--no {params.negative_prompt}"
        if params.seed is not None:
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
            self.logger.error("豆包 Seedream 调用失败: %s", exc)
            raise ImageProviderException(f"豆包 Seedream 调用失败: {exc}", 502) from exc

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
