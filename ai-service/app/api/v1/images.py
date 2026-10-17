"""图像生成 API（Phase 5）。

端点按 docs/16 §7.5 收敛为 `images/generate`，
一次调用完成「解析 → Prompt → 生成 → 落盘」，AI 侧无状态。

BYOK：请求可携带 `llm_credential` / `image_credential`；
有值时按请求构造 Provider，绝不回退系统 Key。
"""

from __future__ import annotations

from fastapi import APIRouter, Depends

from ...models.schemas import ImageGenerateRequest, ImageGenerateResponse
from .deps import build_generation_service, ensure_credential_base_safe, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


@router.post("/generate", response_model=ImageGenerateResponse)
async def generate_image(request: ImageGenerateRequest) -> ImageGenerateResponse:
    """生成一张绘本插画并落盘，返回相对存储路径与图片元信息。"""
    await ensure_credential_base_safe(request)
    service = build_generation_service(request)
    return await service.generate(request)
