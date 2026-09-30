from contextlib import asynccontextmanager

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse

from app.api.v1.router import api_router
from app.utils.config import settings
from app.utils.exceptions import AIServiceException
from app.utils.logger import setup_logging


@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理。"""
    setup_logging()
    yield


def create_app() -> FastAPI:
    """创建 FastAPI 应用。"""
    app = FastAPI(
        title="StoryCanvas AI Service",
        description="基于生成式 AI 的儿童绘本智能创作服务（文本解析 / Prompt 优化 / 图像生成）",
        version="1.0.0",
        lifespan=lifespan,
    )

    # 配置 CORS
    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.cors_origins,
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    # 业务异常统一转为结构化错误（不返回堆栈，避免泄露内部细节）
    @app.exception_handler(AIServiceException)
    async def _ai_exception_handler(_: Request, exc: AIServiceException) -> JSONResponse:
        return JSONResponse(
            status_code=exc.code if 400 <= exc.code < 600 else 500,
            content={"status": "error", "code": exc.code, "message": exc.message},
        )

    # 注册路由
    app.include_router(api_router, prefix="/api/v1")

    return app


app = create_app()
