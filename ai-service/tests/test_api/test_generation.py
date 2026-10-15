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
            "parameters": {"size": "1024x1024"},
        },
    )
    assert resp.status_code == 200
    body = resp.json()

    assert body["status"] == "success"
    assert body["prompt"]
    assert body["parsed_data"] is not None

    image = body["image"]
    assert image["file_path"].startswith("users/u-1/books/b-1/images/generated/")
    assert image["width"] == 1024 and image["height"] == 1024
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
    image = resp.json()["image"]
    assert "unassigned" in image["file_path"]
    # 面积兜底：16x16 远低于真实厂商最小面积（921600），应被等比放大而非报错
    assert image["width"] * image["height"] >= 921600


def test_generate_rejects_empty_text(client: TestClient):
    resp = client.post("/api/v1/images/generate", json={"text": ""})
    assert resp.status_code == 422


def test_generate_rejects_oversize_image_with_readable_reason(client: TestClient, monkeypatch):
    """Q8：超过 MAX_IMAGE_SIZE 的生成图必须给出可读失败原因（透传任务 error_message）。"""
    from app.utils.config import settings

    monkeypatch.setattr(settings, "MAX_IMAGE_SIZE", 1000)
    resp = client.post("/api/v1/images/generate", json={"text": "一只小猫"})

    assert resp.status_code == 502
    body = resp.json()
    assert body["status"] == "error"
    # 文案需说清「为什么失败」：包含大小上限字样、实际大小与上限数值
    # （上限被测试压到 1000 字节，验证 B/KB 级格式化不出现 "0.0MB > 0MB"）
    assert "大小上限" in body["message"]
    assert "1000B" in body["message"]
