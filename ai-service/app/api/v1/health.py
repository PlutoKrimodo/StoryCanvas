from fastapi import APIRouter

from ...utils.config import settings

router = APIRouter()


@router.get("/")
async def health_check():
    """健康检查：返回服务状态与当前生效的 Provider 开关。

    注意：只暴露 Provider 名称，不返回任何密钥信息。
    """
    return {
        "status": "ok",
        "service": "StoryCanvas AI Service",
        "version": "1.0.0",
        "providers": {
            "llm": settings.LLM_PROVIDER,
            "image": settings.IMAGE_PROVIDER,
        },
    }
