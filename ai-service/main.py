import uvicorn

from app.utils.config import settings

if __name__ == "__main__":
    # reload 与多 worker 互斥：开启热重载时强制单进程
    use_reload = settings.DEBUG
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=use_reload,
        workers=1 if use_reload else settings.WORKERS,
    )
