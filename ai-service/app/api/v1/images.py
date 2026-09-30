"""图像生成 API（Phase 5）。

端点按 docs/16 §7.5 收敛为 `images/generate`，
一次调用完成「解析 → Prompt → 生成 → 落盘」，AI 侧无状态。
"""

from __future__ import annotations

from fastapi import APIRouter, Depends

from ...models.schemas import ImageGenerateRequest, ImageGenerateResponse
from ...services.generation_service import GenerationService
from .deps import get_generation_service, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


@router.post("/generate", response_model=ImageGenerateResponse)
async def generate_image(
    request: ImageGenerateRequest,
    service: GenerationService = Depends(get_generation_service),
) -> ImageGenerateResponse:
    """生成一张绘本插画并落盘，返回相对存储路径与图片元信息。"""
    return await service.generate(request)
