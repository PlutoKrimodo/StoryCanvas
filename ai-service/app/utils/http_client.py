"""共享的异步 HTTP 客户端工具。

DeepSeek / 火山方舟 / 通义万相 均为 HTTP JSON API，统一走 httpx，
避免每次调用都新建连接池。
"""

from __future__ import annotations

from typing import Any, Dict, Optional

import httpx

from .logger import get_logger

logger = get_logger(__name__)


async def post_json(
    url: str,
    *,
    headers: Dict[str, str],
    payload: Dict[str, Any],
    timeout: float,
) -> Dict[str, Any]:
    """POST JSON 并返回解析后的字典。

    非 2xx 时抛出 `httpx.HTTPStatusError`，由上层 Provider 包装为业务异常。
    """
    async with httpx.AsyncClient(timeout=timeout) as client:
        response = await client.post(url, headers=headers, json=payload)
        response.raise_for_status()
        return response.json()


async def get_json(
    url: str,
    *,
    headers: Dict[str, str],
    timeout: float,
) -> Dict[str, Any]:
    """GET JSON 并返回解析后的字典。"""
    async with httpx.AsyncClient(timeout=timeout) as client:
        response = await client.get(url, headers=headers)
        response.raise_for_status()
        return response.json()


async def download_bytes(
    url: str,
    *,
    timeout: float,
    headers: Optional[Dict[str, str]] = None,
) -> tuple[bytes, Optional[str]]:
    """下载二进制内容，返回 (内容, Content-Type)。"""
    async with httpx.AsyncClient(timeout=timeout, follow_redirects=True) as client:
        response = await client.get(url, headers=headers or {})
        response.raise_for_status()
        return response.content, response.headers.get("content-type")
