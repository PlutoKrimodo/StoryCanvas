"""凭据测试 API：成功 / 失败可读 / 不泄露 Key / 图像真实 1K 测试。"""

from fastapi.testclient import TestClient

import app.api.v1.credentials as module
from app.utils.exceptions import ImageProviderException, LLMProviderException

PAYLOAD = {
    "capability": "llm",
    "provider": "deepseek",
    "api_key": "sk-very-secret-123456",
    "model": "deepseek-chat",
}


def test_llm_test_success(client: TestClient, monkeypatch):
    class FakeProvider:
        async def generate(self, *args, **kwargs):
            return "pong"

    monkeypatch.setattr(module, "build_llm_provider", lambda credential: FakeProvider())

    resp = client.post("/api/v1/credentials/test", json=PAYLOAD)
    assert resp.status_code == 200
    body = resp.json()
    assert body["ok"] is True
    assert body["verified"] is True
    assert "sk-very-secret" not in resp.text


def test_llm_test_failure_is_readable_and_safe(client: TestClient, monkeypatch):
    class FakeProvider:
        async def generate(self, *args, **kwargs):
            raise LLMProviderException("DeepSeek 调用失败：API Key 无效（HTTP 401）", 401)

    monkeypatch.setattr(module, "build_llm_provider", lambda credential: FakeProvider())

    resp = client.post("/api/v1/credentials/test", json=PAYLOAD)
    assert resp.status_code == 200
    body = resp.json()
    assert body["ok"] is False
    assert "401" in body["message"]
    assert "sk-very-secret" not in resp.text


def test_image_test_generates_a_single_1k_probe(client: TestClient, monkeypatch):
    class FakeImageProvider:
        async def generate(self, params):
            assert params.width == 1024
            assert params.height == 1024
            return None

    monkeypatch.setattr(module, "build_image_provider", lambda credential: FakeImageProvider())

    resp = client.post(
        "/api/v1/credentials/test",
        json={
            "capability": "image",
            "provider": "doubao",
            "api_key": "ark-very-secret-123456",
            "model": "doubao-seedream-5-0-flash-260915",
        },
    )
    assert resp.status_code == 200
    body = resp.json()
    assert body["ok"] is True
    assert body["verified"] is True
    assert "ark-very-secret" not in resp.text


def test_image_test_failure_is_readable_and_safe(client: TestClient, monkeypatch):
    class FakeImageProvider:
        async def generate(self, params):
            raise ImageProviderException("图像调用失败：API Key 无效（HTTP 401）", 401)

    monkeypatch.setattr(module, "build_image_provider", lambda credential: FakeImageProvider())

    resp = client.post(
        "/api/v1/credentials/test",
        json={
            "capability": "image",
            "provider": "doubao",
            "api_key": "ark-very-secret-123456",
            "model": "doubao-seedream-5-0-flash-260915",
        },
    )
    assert resp.status_code == 200
    body = resp.json()
    assert body["ok"] is False
    assert "401" in body["message"]
    assert "ark-very-secret" not in resp.text


def test_unsafe_base_url_rejected_before_calling_vendor(client: TestClient, monkeypatch):
    called = False

    def fake_build(credential):
        nonlocal called
        called = True
        raise AssertionError("不应构造 Provider")

    monkeypatch.setattr(module, "build_llm_provider", fake_build)

    resp = client.post(
        "/api/v1/credentials/test",
        json={**PAYLOAD, "api_base": "http://127.0.0.1:8000/v1"},
    )
    assert resp.status_code == 200
    body = resp.json()
    assert body["ok"] is False
    assert "https" in body["message"]
    assert called is False
