"""BYOK Provider：按请求使用凭据、不回退系统 Key、鉴权失败不重试。"""

import httpx
import pytest

import app.providers.llm.openai_compatible as llm_module
from app.providers.llm.openai_compatible import OpenAICompatibleProvider
from app.services.image_generator import ImageGeneratorService
from app.utils.exceptions import ImageProviderException, LLMProviderException


@pytest.mark.asyncio
async def test_uses_per_request_key_base_and_model(monkeypatch):
    captured = {}

    async def fake_post_json(url, *, headers, payload, timeout):
        captured["url"] = url
        captured["headers"] = headers
        captured["model"] = payload["model"]
        return {"choices": [{"message": {"content": "ok"}}]}

    monkeypatch.setattr(llm_module, "post_json", fake_post_json)
    provider = OpenAICompatibleProvider(
        api_key="sk-user-123456",
        api_base="https://llm.example.com/v1",
        model="my-model",
        provider="custom",
    )

    assert await provider.generate([{"role": "user", "content": "hi"}]) == "ok"
    assert captured["url"] == "https://llm.example.com/v1/chat/completions"
    assert captured["headers"]["Authorization"] == "Bearer sk-user-123456"
    assert captured["model"] == "my-model"


@pytest.mark.asyncio
async def test_never_falls_back_to_system_key(monkeypatch):
    called = False

    async def fake_post_json(*args, **kwargs):
        nonlocal called
        called = True
        return {"choices": [{"message": {"content": "should-not-happen"}}]}

    monkeypatch.setattr(llm_module, "post_json", fake_post_json)

    with pytest.raises(LLMProviderException):
        await OpenAICompatibleProvider(api_key="").generate(
            [{"role": "user", "content": "hi"}]
        )
    assert called is False


@pytest.mark.asyncio
async def test_maps_401_to_fast_fail_and_hides_key(monkeypatch):
    request = httpx.Request(
        "POST",
        "https://llm.example.com/v1/chat/completions",
        headers={"Authorization": "Bearer sk-user-secret"},
    )
    response = httpx.Response(401, request=request, json={"error": "invalid key"})

    async def boom(*args, **kwargs):
        raise httpx.HTTPStatusError("401", request=request, response=response)

    monkeypatch.setattr(llm_module, "post_json", boom)
    provider = OpenAICompatibleProvider(
        api_key="sk-user-secret", api_base="https://llm.example.com/v1",
        model="m", provider="custom",
    )

    with pytest.raises(LLMProviderException) as exc:
        await provider.generate([{"role": "user", "content": "hi"}])

    assert exc.value.code == 401
    assert "401" in exc.value.message
    assert "sk-user-secret" not in exc.value.message


@pytest.mark.asyncio
async def test_image_auth_error_is_not_retried():
    class FakeProvider:
        name = "doubao:test"

        def __init__(self):
            self.calls = 0

        async def generate(self, params):
            self.calls += 1
            raise ImageProviderException("API Key 无效（HTTP 401）", 401)

        async def materialize(self, result):
            raise AssertionError("should not materialize")

    fake = FakeProvider()
    service = ImageGeneratorService(fake)  # type: ignore[arg-type]
    service.max_retries = 3

    with pytest.raises(ImageProviderException):
        await service.generate(object())  # type: ignore[arg-type]

    assert fake.calls == 1
