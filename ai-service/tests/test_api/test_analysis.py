"""文本分析 API 测试（Phase 4 验收）。"""

from fastapi.testclient import TestClient

import app.utils.config as config_module


def test_list_styles(client: TestClient):
    resp = client.get("/api/v1/analysis/styles")
    assert resp.status_code == 200
    keys = [item["key"] for item in resp.json()["styles"]]
    assert "cartoon" in keys and "watercolor" in keys


def test_parse_returns_structured_data(client: TestClient):
    resp = client.post(
        "/api/v1/analysis/parse",
        json={"text": "小女孩在雨后的森林里发现了一只受伤的小鸟。"},
    )
    assert resp.status_code == 200
    body = resp.json()
    parsed = body["parsed_data"]
    assert isinstance(parsed["characters"], list) and parsed["characters"]
    assert "objects" in parsed and "actions" in parsed and "style" in parsed


def test_optimize_with_text(client: TestClient):
    resp = client.post(
        "/api/v1/analysis/optimize",
        json={"text": "小女孩在森林里发现小鸟", "style": "watercolor"},
    )
    assert resp.status_code == 200
    body = resp.json()
    assert body["prompt"]
    assert "watercolor painting" in body["prompt"]
    assert body["style"] == "watercolor"


def test_optimize_with_parsed_data(client: TestClient):
    resp = client.post(
        "/api/v1/analysis/optimize",
        json={
            "style": "cartoon",
            "parsed_data": {
                "characters": [{"name": "小熊"}],
                "scene": {"location": "草地"},
                "objects": ["气球"],
                "actions": ["跑"],
                "emotions": ["开心"],
                "style": {},
            },
        },
    )
    assert resp.status_code == 200
    assert "小熊" in resp.json()["prompt"]


def test_optimize_requires_input(client: TestClient):
    resp = client.post("/api/v1/analysis/optimize", json={"style": "cartoon"})
    assert resp.status_code == 422


def test_empty_text_rejected(client: TestClient):
    resp = client.post("/api/v1/analysis/parse", json={"text": ""})
    assert resp.status_code == 422


def test_internal_key_enforced(client: TestClient, monkeypatch):
    monkeypatch.setattr(config_module.settings, "INTERNAL_API_KEY", "secret-key")

    denied = client.post("/api/v1/analysis/parse", json={"text": "小猫"})
    assert denied.status_code == 401

    allowed = client.post(
        "/api/v1/analysis/parse",
        json={"text": "小猫"},
        headers={"X-Internal-Api-Key": "secret-key"},
    )
    assert allowed.status_code == 200
