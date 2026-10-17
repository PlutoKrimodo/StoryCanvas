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
