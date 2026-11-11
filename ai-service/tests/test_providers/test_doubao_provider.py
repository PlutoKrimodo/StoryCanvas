"""Doubao Seedream Provider 单元测试（monkeypatch，不产生真实网络请求）。"""

import httpx
import pytest

import app.providers.image.doubao_provider as module
from app.models.schemas import ImageGenerationParams
from app.providers.image.doubao_provider import DoubaoProvider
from app.utils.exceptions import (
    AIServiceException,
    CONTENT_REJECTED,
    ImageProviderException,
)


def _params(seed: int | None = None) -> ImageGenerationParams:
    return ImageGenerationParams(
        prompt="a girl on a wooden boat",
        width=1024,
        height=1024,
        seed=seed,
        negative_prompt="split screen, comic strip",
    )


@pytest.mark.asyncio
async def test_seedream_5_omits_seed_n_and_uses_avoid(monkeypatch):
    captured = {}

    async def fake_post_json(url, *, headers, payload, timeout):
        captured["url"] = url
        captured["payload"] = payload
        return {"data": [{"url": "https://example.com/test.png"}]}

    monkeypatch.setattr(module, "post_json", fake_post_json)
    provider = DoubaoProvider(
        api_key="ark-test",
        model="doubao-seedream-5-0-flash-260915",
        api_base="https://ark.example.com/api/v3",
    )

    result = await provider.generate(_params(seed=42))

    payload = captured["payload"]
    assert payload["model"] == "doubao-seedream-5-0-flash-260915"
    assert "seed" not in payload
    assert "n" not in payload
    assert payload["size"] == "1024x1024"
    assert payload["response_format"] == "url"
    assert payload["watermark"] is False
    assert "--no" not in payload["prompt"]
    assert "Avoid:" in payload["prompt"]
    assert result.image_url == "https://example.com/test.png"


@pytest.mark.asyncio
async def test_seedream_3_keeps_seed(monkeypatch):
    captured = {}

    async def fake_post_json(url, *, headers, payload, timeout):
        captured["payload"] = payload
        return {"data": [{"url": "https://example.com/test.png"}]}

    monkeypatch.setattr(module, "post_json", fake_post_json)
    provider = DoubaoProvider(
        api_key="ark-test",
        model="doubao-seedream-3-0-t2i-250415",
        api_base="https://ark.example.com/api/v3",
    )

    await provider.generate(_params(seed=42))

    assert captured["payload"]["seed"] == 42


@pytest.mark.asyncio
async def test_missing_api_key_raises():
    provider = DoubaoProvider(api_key="", model="doubao-seedream-5-0-flash-260915")

    with pytest.raises(ImageProviderException) as exc:
        await provider.generate(_params())

    assert exc.value.code == 503


@pytest.mark.asyncio
async def test_policy_violation_fast_fails_with_readable_message(monkeypatch):
    request = httpx.Request("POST", "https://ark.example.com/api/v3/images/generations")
    response = httpx.Response(
        400,
        request=request,
        json={
            "error": {
                "code": "OutputImageSensitiveContentDetected.PolicyViolation",
                "message": "The request failed because the output image may be related to copyright restrictions.",
            }
        },
    )

    async def fake_post_json(*args, **kwargs):
        raise httpx.HTTPStatusError("bad request", request=request, response=response)

    monkeypatch.setattr(module, "post_json", fake_post_json)
    provider = DoubaoProvider(
        api_key="ark-test",
        model="doubao-seedream-5-0-flash-260915",
        api_base="https://ark.example.com/api/v3",
    )

    with pytest.raises(AIServiceException) as exc:
        await provider.generate(_params())

    assert exc.value.code == 422
    assert exc.value.error_code == CONTENT_REJECTED
    assert "版权" in str(exc.value)
