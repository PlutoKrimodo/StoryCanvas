"""分镜规划 API（Storyboard Chat）。

只规划、不出图：用户确认/编辑分镜后，再由 C++ 后端创建 N 个图像任务。
"""

from __future__ import annotations

from fastapi import APIRouter, Depends

from ...models.schemas import StoryboardRequest, StoryboardResponse
from .deps import build_storyboard_service, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


@router.post("/storyboard", response_model=StoryboardResponse)
async def create_storyboard(request: StoryboardRequest) -> StoryboardResponse:
    """把连续故事拆成 N 格，并返回整组共享的一致性设定。"""
    service = build_storyboard_service(request.llm_credential)
    return await service.generate(request.text, request.image_count, request.style)
