"""图像生成服务（Phase 5）。

在 Provider 之上补齐：超时控制 + 重试 + 结果二进制化（materialize）。
"""

from __future__ import annotations

import asyncio
from typing import Optional, Tuple

from ..models.schemas import ImageGenerationParams, ImageGenerationResult
from ..providers.image.base import ImageProvider
from ..utils.config import settings
from ..utils.exceptions import ImageProviderException
from .base import BaseService


class ImageGeneratorService(BaseService):
    """图像生成服务。"""

    def __init__(self, image_provider: ImageProvider) -> None:
        super().__init__()
        self.image_provider = image_provider
        self.max_retries = max(1, settings.TASK_RETRY_COUNT)
        self.timeout = settings.IMAGE_TIMEOUT

    @property
    def provider_name(self) -> str:
        return self.image_provider.name

    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        """调用 Provider 生成图片，带指数退避重试与超时。"""
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            try:
                return await asyncio.wait_for(
                    self.image_provider.generate(params), timeout=self.timeout
                )
            except asyncio.TimeoutError:
                last_error = f"图像生成超时（第 {attempt}/{self.max_retries} 次）"
                self.logger.warning(last_error)
            except ImageProviderException as exc:
                last_error = f"{exc.message}（第 {attempt}/{self.max_retries} 次）"
                self.logger.warning(last_error)
            except Exception as exc:  # noqa: BLE001 - 统一归集为 Provider 异常
                last_error = f"图像生成失败: {exc}（第 {attempt}/{self.max_retries} 次）"
                self.logger.error(last_error)

            if attempt < self.max_retries:
                await asyncio.sleep(2 ** (attempt - 1))

        raise ImageProviderException(f"图像生成最终失败: {last_error}", 502)

    async def materialize(
        self, result: ImageGenerationResult
    ) -> Tuple[bytes, Optional[str]]:
        """把生成结果统一为二进制内容（供落盘）。"""
        return await self.image_provider.materialize(result)
