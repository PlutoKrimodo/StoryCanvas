"""Mock Image Provider 单元测试。"""

import pytest

from app.models.schemas import ImageGenerationParams
from app.providers.image.mock_provider import MockImageProvider

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


@pytest.mark.asyncio
async def test_generate_returns_real_png_bytes():
    provider = MockImageProvider(delay=0)
    params = ImageGenerationParams(prompt="a cat", width=48, height=32)

    result = await provider.generate(params)

    assert result.image_bytes is not None
    assert result.image_bytes.startswith(PNG_SIGNATURE)
    assert result.content_type == "image/png"
    assert result.width == 48 and result.height == 32
    assert result.seed is not None


@pytest.mark.asyncio
async def test_materialize_prefers_inline_bytes():
    provider = MockImageProvider(delay=0)
    result = await provider.generate(ImageGenerationParams(prompt="x", width=16, height=16))
    data, content_type = await provider.materialize(result)

    assert data == result.image_bytes
    assert content_type == "image/png"


def test_same_prompt_produces_same_seed():
    provider = MockImageProvider(delay=0)
    assert provider.encode((16, 16), 42) == provider.encode((16, 16), 42)
