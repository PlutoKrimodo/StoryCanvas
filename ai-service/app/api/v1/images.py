"""图像生成 API（Phase 5）。

端点按 docs/16 §7.5 收敛为 `images/generate`，
一次调用完成「解析 → Prompt → 生成 → 落盘」，AI 侧无状态。

BYOK：请求可携带 `llm_credential` / `image_credential`；
有值时按请求构造 Provider，绝不回退系统 Key。
"""

from __future__ import annotations

import asyncio

from fastapi import APIRouter, Depends

from ...models.schemas import ImageGenerateRequest, ImageGenerateResponse
from ...utils.exceptions import AIServiceException
from ...utils.url_safety import UnsafeUrlError, validate_public_https_url
from .deps import build_generation_service, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


async def _ensure_credential_base_safe(request: ImageGenerateRequest) -> None:
    """用户自定义 Base URL 必须先通过 SSRF 校验（DNS 解析放到线程池）。"""
    bases = []
    if request.llm_credential is not None and request.llm_credential.api_base:
        bases.append(request.llm_credential.api_base)
    if request.image_credential is not None and request.image_credential.api_base:
        bases.append(request.image_credential.api_base)

    for base in bases:
        try:
            await asyncio.to_thread(validate_public_https_url, base)
        except UnsafeUrlError as exc:
            raise AIServiceException(f"自定义 Base URL 不安全：{exc}", 422) from exc


@router.post("/generate", response_model=ImageGenerateResponse)
async def generate_image(request: ImageGenerateRequest) -> ImageGenerateResponse:
    """生成一张绘本插画并落盘，返回相对存储路径与图片元信息。"""
    await _ensure_credential_base_safe(request)
    service = build_generation_service(request)
    return await service.generate(request)
