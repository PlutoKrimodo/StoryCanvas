"""用户自定义 Base URL 的 SSRF 防护。

只允许公网 HTTPS 域名：
  - 强制 https，禁止 userinfo、查询参数、锚点；
  - 仅允许默认 443 端口；
  - 拒绝 localhost / 单段内网主机名；
  - DNS 解析后逐一校验所有地址，拒绝私网、回环、链路本地、保留与组播地址。

注意：本模块会做 DNS 解析（阻塞），在 async 路由中应通过
`asyncio.to_thread(validate_public_https_url, url)` 调用。
"""

from __future__ import annotations

import ipaddress
import socket
from urllib.parse import urlsplit

_MAX_URL_LENGTH = 500


class UnsafeUrlError(ValueError):
    """Base URL 不安全或不可用。"""


def validate_public_https_url(url: str) -> str:
    """校验并返回去掉尾部 '/' 的 Base URL；不合法时抛 UnsafeUrlError。"""
    text = (url or "").strip()
    if not text:
        raise UnsafeUrlError("Base URL 不能为空")
    if len(text) > _MAX_URL_LENGTH:
        raise UnsafeUrlError("Base URL 过长（最多 500 字符）")
    if not text.startswith("https://"):
        raise UnsafeUrlError("Base URL 必须以 https:// 开头")

    try:
        parts = urlsplit(text)
        port = parts.port
    except ValueError as exc:
        raise UnsafeUrlError("Base URL 格式非法") from exc

    if parts.scheme != "https":
        raise UnsafeUrlError("Base URL 必须以 https:// 开头")
    if parts.username or parts.password:
        raise UnsafeUrlError("Base URL 不允许包含用户名或密码")
    if port not in (None, 443):
        raise UnsafeUrlError("Base URL 仅支持 443 端口")
    if parts.query or parts.fragment:
        raise UnsafeUrlError("Base URL 不允许包含查询参数或锚点")

    host = (parts.hostname or "").strip().lower()
    if not host:
        raise UnsafeUrlError("Base URL 缺少域名")
    if host == "localhost" or host.endswith(".localhost"):
        raise UnsafeUrlError("Base URL 不允许指向 localhost")
    if "." not in host:
        raise UnsafeUrlError("Base URL 必须是完整域名，不允许单段内网主机名")
    try:
        ipaddress.ip_address(host)
    except ValueError:
        pass
    else:
        raise UnsafeUrlError("Base URL 不允许使用 IP 地址")

    try:
        infos = socket.getaddrinfo(host, 443, type=socket.SOCK_STREAM)
    except OSError as exc:
        raise UnsafeUrlError("Base URL 域名无法解析，请检查地址是否正确") from exc

    addresses = {info[4][0] for info in infos}
    if not addresses:
        raise UnsafeUrlError("Base URL 域名无法解析，请检查地址是否正确")

    for address in addresses:
        try:
            ip = ipaddress.ip_address(address)
        except ValueError as exc:
            raise UnsafeUrlError("Base URL 域名解析结果非法") from exc
        if not ip.is_global:
            raise UnsafeUrlError("Base URL 不允许指向内网、回环或保留地址")

    return text.rstrip("/")
