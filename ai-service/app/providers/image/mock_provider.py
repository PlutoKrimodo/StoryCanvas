"""Mock 图像 Provider（开发 / 测试 / 故障兜底）。

**刻意不生成本地假图，而是生成一张真实的 PNG 渐变图**，原因：
1. 让「落盘 → 后端记录 → 前端预览 → PDF 导出」整条链路在离线环境也能真跑通；
2. 避免依赖 Pillow 等额外依赖（用 zlib + struct 手写最小 PNG 编码）。
"""

from __future__ import annotations

import asyncio
import hashlib
import random
import struct
import zlib
from typing import Tuple

from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings
from .base import ImageProvider


def _seed_from(text: str, seed: int | None) -> int:
    if seed is not None:
        return int(seed) & 0x7FFFFFFF
    digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
    return int(digest[:8], 16)


def _png_bytes(width: int, height: int, seed: int) -> bytes:
    """生成一张对角线渐变 PNG（纯 Python，无第三方依赖）。"""
    width = max(1, min(int(width), 2048))
    height = max(1, min(int(height), 2048))

    rnd = random.Random(seed)
    c1 = (rnd.randint(60, 220), rnd.randint(60, 220), rnd.randint(90, 255))
    c2 = (rnd.randint(40, 180), rnd.randint(60, 220), rnd.randint(120, 255))

    rows = []
    denom_x = max(width - 1, 1)
    denom_y = max(height - 1, 1)
    for y in range(height):
        ty = y / denom_y
        row = bytearray((0,))  # filter type 0
        for x in range(width):
            t = (x / denom_x + ty) / 2
            row.append(int(c1[0] * (1 - t) + c2[0] * t))
            row.append(int(c1[1] * (1 - t) + c2[1] * t))
            row.append(int(c1[2] * (1 - t) + c2[2] * t))
        rows.append(bytes(row))
    raw = b"".join(rows)
    compressed = zlib.compress(raw, 6)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8bit RGB
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", compressed)
        + chunk(b"IEND", b"")
    )


class MockImageProvider(ImageProvider):
    """离线可用的 Mock 实现。"""

    def __init__(self, delay: float | None = None) -> None:
        super().__init__()
        self.delay = settings.MOCK_IMAGE_DELAY if delay is None else delay

    @property
    def name(self) -> str:
        return "mock-image"

    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        if self.delay and self.delay > 0:
            await asyncio.sleep(self.delay)

        seed = _seed_from(params.prompt, params.seed)
        data = _png_bytes(params.width, params.height, seed)
        return ImageGenerationResult(
            image_url="",
            width=params.width,
            height=params.height,
            seed=seed,
            finish_reason="success",
            image_bytes=data,
            content_type="image/png",
        )

    async def health_check(self) -> bool:
        return True

    @staticmethod
    def encode(size: Tuple[int, int], seed: int = 1) -> bytes:
        """供测试直接调用：生成指定尺寸的 PNG。"""
        return _png_bytes(size[0], size[1], seed)
