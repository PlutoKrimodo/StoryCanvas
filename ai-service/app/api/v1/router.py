from fastapi import APIRouter

from . import analysis, health, images

api_router = APIRouter()

# 健康检查（无需内部鉴权）
api_router.include_router(
    health.router,
    prefix="/health",
    tags=["Health"],
)

# 文本分析（Phase 4）
api_router.include_router(
    analysis.router,
    prefix="/analysis",
    tags=["Analysis"],
)

# 图像生成（Phase 5）
api_router.include_router(
    images.router,
    prefix="/images",
    tags=["Images"],
)
