"""图像生成 API 测试（Phase 5 验收）。"""

from fastapi.testclient import TestClient


def test_generate_image_and_persist(client: TestClient, storage_root):
    resp = client.post(
        "/api/v1/images/generate",
        json={
            "text": "小女孩在雨后的森林里发现了一只受伤的小鸟。",
            "style": "cartoon",
            "user_id": "u-1",
            "book_id": "b-1",
            "parameters": {"width": 256, "height": 256},
        },
    )
    assert resp.status_code == 200
    body = resp.json()

    assert body["status"] == "success"
    assert body["prompt"]
    assert body["parsed_data"] is not None

    image = body["image"]
    assert image["file_path"].startswith("users/u-1/books/b-1/images/generated/")
    assert image["width"] == 256 and image["height"] == 256
    assert image["file_size"] > 0

    saved = storage_root / image["file_path"]
    assert saved.exists(), "生成图必须落盘"
    assert saved.read_bytes().startswith(b"\x89PNG")


def test_generate_defaults_book_to_unassigned(client: TestClient, storage_root):
    resp = client.post(
        "/api/v1/images/generate",
        json={"text": "一只小猫", "parameters": {"size": "16x16"}},
    )
    assert resp.status_code == 200
    file_path = resp.json()["image"]["file_path"]
    assert "unassigned" in file_path


def test_generate_rejects_empty_text(client: TestClient):
    resp = client.post("/api/v1/images/generate", json={"text": ""})
    assert resp.status_code == 422
