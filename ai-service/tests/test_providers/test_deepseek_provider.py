"""DeepSeek Provider 单元测试（用 monkeypatch 替身，不产生真实网络请求）。"""

import pytest

import app.providers.llm.deepseek_provider as module
from app.providers.llm.deepseek_provider import DeepSeekProvider
from app.utils.exceptions import LLMProviderException


@pytest.mark.asyncio
async def test_generate_calls_chat_completions(monkeypatch):
    captured = {}

    async def fake_post_json(url, *, headers, payload, timeout):
        captured["url"] = url
        captured["payload"] = payload
        return {"choices": [{"message": {"content": "hello"}}]}

    monkeypatch.setattr(module, "post_json", fake_post_json)

    provider = DeepSeekProvider(api_key="test-key", model="deepseek-chat")
    content = await provider.generate([{"role": "user", "content": "hi"}], json_mode=True)

    assert content == "hello"
    assert captured["url"].endswith("/chat/completions")
    assert captured["payload"]["response_format"] == {"type": "json_object"}
    assert captured["payload"]["model"] == "deepseek-chat"


@pytest.mark.asyncio
async def test_generate_without_api_key_raises():
    provider = DeepSeekProvider(api_key="")
    with pytest.raises(LLMProviderException):
        await provider.generate([{"role": "user", "content": "hi"}])


@pytest.mark.asyncio
async def test_generate_wraps_transport_error(monkeypatch):
    async def boom(*args, **kwargs):
        raise RuntimeError("connection reset")

    monkeypatch.setattr(module, "post_json", boom)
    provider = DeepSeekProvider(api_key="test-key")

    with pytest.raises(LLMProviderException) as exc:
        await provider.generate([{"role": "user", "content": "hi"}])
    assert "DeepSeek 调用失败" in exc.value.message
