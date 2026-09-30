"""Image Provider 抽象。"""

from __future__ import annotations

from abc import abstractmethod
from typing import Optional, Tuple

from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings
from ...utils.http_client import download_bytes
from ..base import BaseProvider


class ImageProvider(BaseProvider):
    """图像生成 Provider 基类。"""

    @abstractmethod
    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        """根据参数生成一张图片。"""

    async def materialize(
        self, result: ImageGenerationResult
    ) -> Tuple[bytes, Optional[str]]:
        """把生成结果统一转换为 (二进制内容, Content-Type)。

        - Provider 直接返回二进制时（如 Mock）直接使用；
        - 否则按 `image_url` 下载。默认实现对所有真实厂商通用。
        """
        if result.image_bytes is not None:
            return result.image_bytes, result.content_type
        if not result.image_url:
            raise ValueError("生成结果既无 image_bytes 也无 image_url")
        return await download_bytes(
            result.image_url,
            timeout=settings.IMAGE_TIMEOUT,
            headers={"User-Agent": "StoryCanvas-AI-Service/1.0"},
        )
