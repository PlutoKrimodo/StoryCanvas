"""本地文件存储工具。

目录结构（对照 docs/09 文件存储方案 §2.1）：

    {STORAGE_PATH}/users/{user_id}/books/{book_id}/images/generated/{image_id}.{ext}

AI 服务与 C++ 后端共享同一存储卷，因此这里写入的 **相对路径** 会被后端直接落库，
由后端通过 `/api/v1/images/{image_id}/raw` 读取并回给前端。
"""

from __future__ import annotations

import os
import uuid
from pathlib import Path
from typing import Optional

from .config import settings
from .logger import get_logger

logger = get_logger(__name__)

# 内容类型 → 扩展名（用于推断落盘文件名）
_CONTENT_TYPE_EXT = {
    "image/png": "png",
    "image/jpeg": "jpg",
    "image/jpg": "jpg",
    "image/webp": "webp",
    "image/gif": "gif",
}


def storage_root() -> Path:
    """存储根目录（绝对路径）。"""
    return Path(settings.STORAGE_PATH).expanduser().resolve()


def build_image_rel_path(
    user_id: Optional[str],
    book_id: Optional[str],
    image_id: str,
    extension: str,
) -> str:
    """构造生成图的相对存储路径。

    未归属绘本时（如「先生成、后保存」的解耦流程）使用 `unassigned` 占位，
    保证匿名 / 未绑定场景也能落盘。
    """
    user = (user_id or "unassigned").strip() or "unassigned"
    book = (book_id or "unassigned").strip() or "unassigned"
    ext = (extension or "png").lstrip(".").lower()
    return f"users/{user}/books/{book}/images/generated/{image_id}.{ext}"


def guess_extension(content_type: Optional[str], url: Optional[str]) -> str:
    """从 Content-Type 或 URL 后缀推断图片扩展名，默认 png。"""
    if content_type:
        base = content_type.split(";")[0].strip().lower()
        if base in _CONTENT_TYPE_EXT:
            return _CONTENT_TYPE_EXT[base]
    if url:
        suffix = Path(url.split("?")[0]).suffix.lstrip(".").lower()
        if suffix in {"png", "jpg", "jpeg", "webp", "gif"}:
            return "jpg" if suffix == "jpeg" else suffix
    return "png"


def save_bytes(rel_path: str, data: bytes) -> int:
    """把二进制内容写入存储，返回写入字节数。

    `rel_path` 必须是由本模块构造的相对路径；这里会做一次越界校验，
    拒绝 `..` 等可能逃逸出存储根目录的路径。
    """
    target = (storage_root() / rel_path).resolve()
    root = storage_root()
    if not str(target).startswith(str(root) + os.sep) and target != root:
        raise ValueError(f"非法的存储路径: {rel_path}")

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    logger.debug("生成图已落盘: %s (%d 字节)", target, len(data))
    return len(data)


def new_image_id() -> str:
    """生成图片唯一 ID（UUID v4，无连字符便于作文件名）。"""
    return uuid.uuid4().hex


def read_bytes(rel_path: str) -> bytes:
    """读取存储中的文件（测试 / 兜底用）。"""
    return (storage_root() / rel_path).read_bytes()


def remove_file(rel_path: str) -> bool:
    """删除存储中的文件，不存在时返回 False。"""
    target = storage_root() / rel_path
    try:
        target.unlink()
        return True
    except FileNotFoundError:
        return False
