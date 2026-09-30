"""pytest 全局配置。

关键点：必须在导入 `app.*` **之前** 注入环境变量，
因为 `app.utils.config.settings` 在模块导入时即完成实例化。
"""

from __future__ import annotations

import os
import shutil
import tempfile

# ---- 1. 强制离线可测的 Provider 组合 ----
os.environ["LLM_PROVIDER"] = "mock"
os.environ["IMAGE_PROVIDER"] = "mock"
os.environ["MOCK_IMAGE_DELAY"] = "0"
os.environ["TASK_RETRY_COUNT"] = "1"
os.environ["INTERNAL_API_KEY"] = ""  # 单测关闭内部鉴权
os.environ.setdefault("CORS_ORIGINS", "http://localhost:5173")

# ---- 2. 独立的临时存储目录（避免污染仓库 storage/） ----
_TEST_STORAGE = tempfile.mkdtemp(prefix="storycanvas-test-storage-")
os.environ["STORAGE_PATH"] = _TEST_STORAGE

import pytest  # noqa: E402
from fastapi.testclient import TestClient  # noqa: E402

from app.api.v1.deps import reset_provider_cache  # noqa: E402
from app.main import app  # noqa: E402
from app.utils import storage  # noqa: E402


@pytest.fixture(scope="session")
def client() -> TestClient:
    """FastAPI 测试客户端。"""
    return TestClient(app)


@pytest.fixture(autouse=True)
def _reset_caches():
    """每个用例前清空 Provider 缓存，避免用例间互相影响。"""
    reset_provider_cache()
    yield
    reset_provider_cache()


@pytest.fixture(scope="session", autouse=True)
def _cleanup_storage():
    yield
    shutil.rmtree(_TEST_STORAGE, ignore_errors=True)


@pytest.fixture()
def storage_root():
    return storage.storage_root()
