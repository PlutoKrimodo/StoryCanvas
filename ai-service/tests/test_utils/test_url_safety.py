"""自定义 Base URL 的 SSRF 校验测试。"""

import socket

import pytest

import app.utils.url_safety as module
from app.utils.url_safety import UnsafeUrlError, validate_public_https_url


def _resolve_to(monkeypatch, address: str):
    def fake_getaddrinfo(host, port, **kwargs):
        return [(socket.AF_INET, socket.SOCK_STREAM, 6, "", (address, port))]

    monkeypatch.setattr(module.socket, "getaddrinfo", fake_getaddrinfo)


def test_accepts_public_https_domain(monkeypatch):
    _resolve_to(monkeypatch, "93.184.216.34")

    assert validate_public_https_url("https://api.example.com/v1/") == (
        "https://api.example.com/v1"
    )


@pytest.mark.parametrize(
    "url",
    [
        "http://api.example.com/v1",
        "https://user:pass@api.example.com/v1",
        "https://api.example.com:8443/v1",
        "https://api.example.com/v1?debug=1",
        "https://localhost/v1",
        "https://internal/v1",
        "https://93.184.216.34/v1",
        "",
    ],
)
def test_rejects_unsafe_syntax(monkeypatch, url):
    _resolve_to(monkeypatch, "93.184.216.34")

    with pytest.raises(UnsafeUrlError):
        validate_public_https_url(url)


@pytest.mark.parametrize(
    "address",
    [
        "127.0.0.1",
        "10.0.0.8",
        "172.16.0.10",
        "192.168.1.2",
        "169.254.169.254",
        "0.0.0.0",
        "::1",
        "fc00::1",
        "fe80::1",
    ],
)
def test_rejects_private_or_reserved_resolution(monkeypatch, address):
    _resolve_to(monkeypatch, address)

    with pytest.raises(UnsafeUrlError):
        validate_public_https_url("https://api.example.com/v1")


def test_rejects_unresolvable_host(monkeypatch):
    def boom(host, port, **kwargs):
        raise socket.gaierror("no such host")

    monkeypatch.setattr(module.socket, "getaddrinfo", boom)

    with pytest.raises(UnsafeUrlError):
        validate_public_https_url("https://api.example.com/v1")
