import json
from typing import List

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
    # 声明为 str：pydantic-settings 对「复合类型」字段会先尝试 JSON 解码，
    # 逗号分隔写法会在校验器介入前直接报 SettingsError。这里统一按字符串收口，
    # 再由 cors_origins 属性解析（兼容 JSON 数组与逗号分隔两种写法）。
    CORS_ORIGINS: str = "http://localhost:5173,http://localhost:8080"

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
    # 默认指向仓库根 storage/：AI 服务从 ai-service/ 启动，故为 ../storage。
    # **必须与后端 STORAGE_PATH 一致**，否则后端读不到本服务落盘的生成图（见开发记录问题 18）。
    # docker-compose 会以 STORAGE_PATH=/app/storage 覆盖。
    STORAGE_PATH: str = "../storage"
    # 单张生成图大小上限（超过则拒绝落盘，防止异常大文件）
    MAX_IMAGE_SIZE: int = 10 * 1024 * 1024  # 10MB

    # ==================== 任务配置 ====================
    MAX_CONCURRENT_TASKS: int = 5
    TASK_RETRY_COUNT: int = 3
    # Mock 图像 Provider 的模拟耗时（秒），便于观察前端轮询过程；测试中可置 0
    MOCK_IMAGE_DELAY: float = 1.5

    # ==================== 内部鉴权 ====================
    INTERNAL_API_KEY: str = ""

    @property
    def cors_origins(self) -> List[str]:
        """解析 CORS 允许来源，兼容 JSON 数组与逗号分隔两种写法。

        例：`["http://a.com"]` 或 `http://a.com,http://b.com`。
        """
        text = (self.CORS_ORIGINS or "").strip()
        if not text:
            return []
        if text.startswith("["):
            try:
                parsed = json.loads(text)
                if isinstance(parsed, list):
                    return [str(item).strip() for item in parsed if str(item).strip()]
            except json.JSONDecodeError:
                pass
        return [item.strip() for item in text.split(",") if item.strip()]


settings = Settings()
