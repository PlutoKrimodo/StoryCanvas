"""分镜 API：Mock 兜底可离线跑通，且内部鉴权生效。"""

from fastapi.testclient import TestClient

import app.utils.config as config_module


def test_storyboard_returns_requested_panel_count(client: TestClient):
    resp = client.post(
        "/api/v1/analysis/storyboard",
        json={
            "text": "小女孩在雨后的森林里发现一只受伤的小鸟。她把它带回家。第二天小鸟飞走了。",
            "image_count": 3,
            "style": "watercolor",
        },
    )
    assert resp.status_code == 200
    body = resp.json()
    assert len(body["panels"]) == 3
    assert [item["index"] for item in body["panels"]] == [0, 1, 2]
    assert body["parsed_data"] is not None
    assert body["consistency_prompt"]
    assert body["negative_prompt"]


def test_storyboard_accepts_byok_llm_credential(client: TestClient):
    """BYOK 口径：分镜阶段按请求构造 Provider（mock 用于可预测行为）。"""
    resp = client.post(
        "/api/v1/analysis/storyboard",
        json={
            "text": "小女孩在雨后的森林里发现一只受伤的小鸟。",
            "image_count": 1,
            "llm_credential": {
                "provider": "mock",
                "api_key": "sk-very-secret-123456",
            },
        },
    )
    assert resp.status_code == 200
    assert len(resp.json()["panels"]) == 1


def test_storyboard_rejects_unsafe_llm_base(client: TestClient):
    """分镜与出图同一套 SSRF 规则：自定义 Base URL 必须是公网 HTTPS。"""
    resp = client.post(
        "/api/v1/analysis/storyboard",
        json={
            "text": "一只小猫",
            "image_count": 1,
            "llm_credential": {
                "provider": "custom",
                "api_key": "sk-very-secret-123456",
                "api_base": "http://127.0.0.1:8080/v1",
            },
        },
    )
    assert resp.status_code == 422
    assert "不安全" in resp.json()["detail"]


def test_storyboard_enforces_internal_key(client: TestClient, monkeypatch):
    monkeypatch.setattr(config_module.settings, "INTERNAL_API_KEY", "secret-key")

    denied = client.post(
        "/api/v1/analysis/storyboard",
        json={"text": "一只小猫", "image_count": 1},
    )
    assert denied.status_code == 401

    allowed = client.post(
        "/api/v1/analysis/storyboard",
        json={"text": "一只小猫", "image_count": 1},
        headers={"X-Internal-Api-Key": "secret-key"},
    )
    assert allowed.status_code == 200
    assert len(allowed.json()["panels"]) == 1
