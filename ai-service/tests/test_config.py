"""启动期配置校验（内部鉴权 fail-closed）。

`_env_file=None` 屏蔽本地 `ai-service/.env`，保证断言的是代码默认值而非开发机配置。
"""

from __future__ import annotations

import pytest
from pydantic import ValidationError

from app.utils.config import Settings


def build(**overrides) -> Settings:
    return Settings(_env_file=None, **overrides)


def test_production_rejects_missing_or_placeholder_key():
    for bad_key in ("", "change_me_internal_key", "Your_Key", "  "):
        with pytest.raises(ValidationError, match="INTERNAL_API_KEY"):
            build(ENVIRONMENT="production", INTERNAL_API_KEY=bad_key, HOST="127.0.0.1")


def test_production_accepts_real_key():
    assert build(ENVIRONMENT="production", INTERNAL_API_KEY="real-internal-key")


def test_empty_key_refuses_non_loopback_bind():
    for host in ("0.0.0.0", "192.168.1.20"):
        with pytest.raises(ValidationError, match="回环"):
            build(ENVIRONMENT="development", INTERNAL_API_KEY="", HOST=host)


def test_empty_key_allowed_on_loopback():
    assert build(ENVIRONMENT="development", INTERNAL_API_KEY="", HOST="127.0.0.1")
    assert build(ENVIRONMENT="development", INTERNAL_API_KEY="", HOST="localhost")


def test_host_defaults_to_loopback():
    assert build().HOST == "127.0.0.1"
