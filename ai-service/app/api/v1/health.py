from fastapi import APIRouter

router = APIRouter()


@router.get("/")
async def health_check():
    """健康检查"""
    return {
        "status": "ok",
        "service": "StoryCanvas AI Service",
        "version": "1.0.0"
    }
