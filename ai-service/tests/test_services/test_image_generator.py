"""图像生成服务单元测试（超时 / 重试 / materialize）。"""

import pytest

from app.models.schemas import ImageGenerationParams, ImageGenerationResult
from app.providers.image.mock_provider import MockImageProvider
from app.services.image_generator import ImageGeneratorService
from app.utils.exceptions import ImageProviderException


@pytest.mark.asyncio
async def test_generate_and_materialize():
    service = ImageGeneratorService(MockImageProvider(delay=0))
    result = await service.generate(ImageGenerationParams(prompt="a dog", width=32, height=32))

    data, content_type = await service.materialize(result)
    assert data.startswith(b"\x89PNG")
    assert content_type == "image/png"
    assert service.provider_name == "mock-image"


@pytest.mark.asyncio
async def test_generate_retries_then_raises():
    class AlwaysFailProvider(MockImageProvider):
        def __init__(self):
            super().__init__(delay=0)
            self.calls = 0

        async def generate(self, params):  # type: ignore[override]
            self.calls += 1
            raise RuntimeError("provider down")

    provider = AlwaysFailProvider()
    service = ImageGeneratorService(provider)
    service.max_retries = 2

    with pytest.raises(ImageProviderException):
        await service.generate(ImageGenerationParams(prompt="x", width=8, height=8))
    assert provider.calls == 2


@pytest.mark.asyncio
async def test_materialize_downloads_when_no_inline_bytes(monkeypatch):
    """Provider 只返回 URL 时，materialize 应回落到下载。"""

    async def fake_download(url, *, timeout, headers=None):
        return b"downloaded", "image/jpeg"

    import app.providers.image.base as base_module

    monkeypatch.setattr(base_module, "download_bytes", fake_download)

    provider = MockImageProvider(delay=0)
    result = ImageGenerationResult(image_url="https://example.com/a.jpg", width=10, height=10)
    data, content_type = await provider.materialize(result)

    assert data == b"downloaded"
    assert content_type == "image/jpeg"
