from fastapi import APIRouter
from . import health

api_router = APIRouter()

# 注册路由
api_router.include_router(
    health.router,
    prefix="/health",
    tags=["Health"]
)
