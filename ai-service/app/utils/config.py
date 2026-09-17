from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    """配置管理"""

    # 应用配置
    APP_NAME: str = "StoryCanvas AI Service"
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    DEBUG: bool = False
    WORKERS: int = 4

    # CORS 配置
    CORS_ORIGINS: List[str] = ["http://localhost:5173", "http://localhost:8080"]

    # AI 服务配置
    OPENAI_API_KEY: str = ""
    OPENAI_API_BASE: str = "https://api.openai.com/v1"
    OPENAI_MODEL: str = "gpt-3.5-turbo"

    STABILITY_API_KEY: str = ""
    STABILITY_API_HOST: str = "https://api.stability.ai"

    # 超时配置
    LLM_TIMEOUT: int = 30
    IMAGE_TIMEOUT: int = 120

    # 存储配置
    STORAGE_PATH: str = "./storage"
    MAX_IMAGE_SIZE: int = 10 * 1024 * 1024

    # 任务配置
    MAX_CONCURRENT_TASKS: int = 5
    TASK_RETRY_COUNT: int = 3

    # 内部 API 配置
    INTERNAL_API_KEY: str = ""

    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"


settings = Settings()
