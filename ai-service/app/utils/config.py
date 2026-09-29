import json
from typing import List

from pydantic import field_validator
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """AI Service 配置

    密钥类配置（DEEPSEEK_API_KEY、ARK_API_KEY、DASHSCOPE_API_KEY、INTERNAL_API_KEY）
    一律由环境变量或 .env 注入，禁止硬编码。
    """

    # pydantic v2 写法（`class Config` 在 v2 已废弃）
    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        case_sensitive=True,
        extra="ignore",
    )

    # ==================== 应用配置 ====================
    APP_NAME: str = "StoryCanvas AI Service"
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    DEBUG: bool = False
    WORKERS: int = 4

    # ==================== CORS ====================
    # 支持两种写法：JSON 数组或逗号分隔字符串
    CORS_ORIGINS: List[str] = ["http://localhost:5173", "http://localhost:8080"]

    # ==================== AI Provider 开关 ====================
    # 演示默认走真实厂商；开发/测试/故障兜底可切 mock
    LLM_PROVIDER: str = "deepseek"   # deepseek | mock
    IMAGE_PROVIDER: str = "doubao"   # doubao | wanx | mock

    # ==================== LLM 配置（DeepSeek）====================
    DEEPSEEK_API_KEY: str = ""
    DEEPSEEK_API_BASE: str = "https://api.deepseek.com"
    DEEPSEEK_MODEL: str = "deepseek-chat"

    # ==================== 图像生成配置 ====================
    # 主用：豆包·Seedream（火山方舟）
    ARK_API_KEY: str = ""
    ARK_API_BASE: str = "https://ark.cn-beijing.volces.com/api/v3"
    ARK_IMAGE_MODEL: str = "doubao-seedream-4-0-xxxx"
    # 备用：通义万相（阿里云百炼）
    DASHSCOPE_API_KEY: str = ""
    DASHSCOPE_API_BASE: str = "https://dashscope.aliyuncs.com"
    WANX_IMAGE_MODEL: str = "wan2.6-image"

    # ==================== 超时配置 ====================
    LLM_TIMEOUT: int = 30
    IMAGE_TIMEOUT: int = 120

    # ==================== 存储配置 ====================
    STORAGE_PATH: str = "./storage"

    # ==================== 任务配置 ====================
    MAX_CONCURRENT_TASKS: int = 5
    TASK_RETRY_COUNT: int = 3

    # ==================== 内部鉴权 ====================
    INTERNAL_API_KEY: str = ""

    @field_validator("CORS_ORIGINS", mode="before")
    @classmethod
    def _parse_origins(cls, value):
        """兼容 JSON 数组字符串与逗号分隔字符串两种环境变量写法"""
        if isinstance(value, str):
            text = value.strip()
            if not text:
                return []
            if text.startswith("["):
                return json.loads(text)
            return [item.strip() for item in text.split(",") if item.strip()]
        return value


settings = Settings()
