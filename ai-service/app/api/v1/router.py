from fastapi import APIRouter

from . import analysis, credentials, health, images, storyboard

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

# BYOK 凭据测试
api_router.include_router(
    credentials.router,
    prefix="/credentials",
    tags=["Credentials"],
)

# 分镜规划（对话式多图生成）
api_router.include_router(
    storyboard.router,
    prefix="/analysis",
    tags=["Storyboard"],
)
