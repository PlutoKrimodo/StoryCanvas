"""共享的异步 HTTP 客户端工具。

DeepSeek / 火山方舟 / 通义万相 均为 HTTP JSON API，统一走 httpx，
避免每次调用都新建连接池。
"""

from __future__ import annotations

from typing import Any, Dict, Optional

import httpx

from .logger import get_logger

logger = get_logger(__name__)


def describe_http_error(exc: Exception) -> tuple[int | None, str]:
    """把 httpx 异常归一为（上游状态码, 可读中文原因）。

    不拼接响应体 / 请求头，避免厂商回显请求内容时把 API Key 带进日志或前端。
    """
    if isinstance(exc, httpx.HTTPStatusError):
        status = exc.response.status_code
        hints = {
            400: "请求参数不被厂商接受",
            401: "API Key 无效或未授权",
            402: "账户余额不足",
            403: "API Key 无权限或已被禁用",
            404: "接口地址或模型不存在",
            429: "请求过于频繁或额度已用尽",
        }
        if status >= 500:
            return status, f"厂商服务暂时不可用（HTTP {status}）"
        return status, f"{hints.get(status, '调用失败')}（HTTP {status}）"
    if isinstance(exc, httpx.TimeoutException):
        return None, "调用厂商接口超时"
    if isinstance(exc, httpx.ConnectError):
        return None, "无法连接厂商接口（请检查 Base URL 或网络）"
    return None, "调用厂商接口失败"


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
