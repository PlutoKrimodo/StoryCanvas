from enum import Enum
from typing import Any, Dict, List, Optional

from pydantic import BaseModel, ConfigDict, Field


class TaskStatus(str, Enum):
    """生成任务状态（与 sql/init.sql 的 CHECK 约束保持一致）"""

    PENDING = "pending"
    PROCESSING = "processing"
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"


class ArtStyle(str, Enum):
    """艺术风格"""

    CARTOON = "cartoon"
    WATERCOLOR = "watercolor"
    OIL_PAINTING = "oil_painting"
    ANIME = "anime"
    PENCIL_SKETCH = "pencil_sketch"


class ParsedCharacter(BaseModel):
    """解析后的角色"""

    name: str
    description: str = ""
    appearance: str = ""
    position: str = ""


class ParsedScene(BaseModel):
    """解析后的场景"""

    location: str = ""
    time: str = ""
    weather: str = ""
    mood: str = ""


class ParsedData(BaseModel):
    """解析后的结构化数据"""

    characters: List[ParsedCharacter] = Field(default_factory=list)
    scene: ParsedScene = Field(default_factory=ParsedScene)
    objects: List[str] = Field(default_factory=list)
    actions: List[str] = Field(default_factory=list)
    emotions: List[str] = Field(default_factory=list)
    style: Dict[str, Any] = Field(default_factory=dict)


class TextAnalysisRequest(BaseModel):
    """文本解析请求（Phase 4）"""

    text: str = Field(..., min_length=1, max_length=2000, description="待解析的故事文本")


class PromptOptimizeRequest(BaseModel):
    """Prompt 优化请求（Phase 4）

    可直接传文本（内部先解析），也可直接传已解析的结构化数据。
    两者至少提供一个。
    """

    text: Optional[str] = Field(None, max_length=2000, description="故事文本（内部先解析）")
    parsed_data: Optional[ParsedData] = Field(None, description="已解析的结构化数据")
    style: ArtStyle = Field(default=ArtStyle.CARTOON, description="艺术风格")
    additional_params: Dict[str, Any] = Field(default_factory=dict, description="附加参数")


class PromptOptimizeResponse(BaseModel):
    """Prompt 优化响应（Phase 4）"""

    prompt: str
    negative_prompt: str = ""
    style: ArtStyle = ArtStyle.CARTOON
    parsed_data: ParsedData


class LLMCredential(BaseModel):
    """用户自带的 LLM 凭据（BYOK）。

    仅存在于单次请求内存；Provider 按请求显式构造，绝不进入 `lru_cache`。
    """

    provider: str = Field(..., min_length=1, max_length=32, description="deepseek/openai/...")
    api_key: str = Field(..., min_length=8, max_length=200)
    api_base: Optional[str] = Field(None, max_length=500)
    model: Optional[str] = Field(None, max_length=120)


class ImageCredential(BaseModel):
    """用户自带的图像凭据（BYOK）：仅支持豆包 / 通义万相两种协议。"""

    provider: str = Field(..., pattern="^(doubao|wanx)$")
    api_key: str = Field(..., min_length=8, max_length=200)
    api_base: Optional[str] = Field(None, max_length=500)
    model: Optional[str] = Field(None, max_length=120)


class CredentialTestRequest(BaseModel):
    """凭据测试请求（由 C++ 后端转发）。"""

    capability: str = Field(..., pattern="^(llm|image)$")
    provider: str = Field(..., min_length=1, max_length=32)
    api_key: str = Field(..., min_length=8, max_length=200)
    api_base: Optional[str] = Field(None, max_length=500)
    model: Optional[str] = Field(None, max_length=120)


class CredentialTestResponse(BaseModel):
    """凭据测试结果。

    业务失败（Key 无效 / 额度不足）也返回 200 + ok=false，
    便于后端直接展示可读原因；只有请求本身非法才由 FastAPI 返回 4xx。
    """

    ok: bool
    verified: bool = False
    latency_ms: int = 0
    message: str = ""


class StoryboardCharacter(BaseModel):
    """分镜一致性设定中的角色外观。"""

    name: str = Field(..., min_length=1, max_length=50)
    appearance: str = Field("", max_length=300)


class StoryboardConsistency(BaseModel):
    """整组图片共享的角色 / 配色 / 画风约束。"""

    characters: List[StoryboardCharacter] = Field(default_factory=list)
    palette: str = Field("", max_length=120)
    notes: str = Field("", max_length=300)


class StoryboardPanel(BaseModel):
    """一格的画面描述（从 0 开始编号）。"""

    index: int = Field(..., ge=0, le=20)
    text: str = Field(..., min_length=1, max_length=500)


class StoryboardRequest(BaseModel):
    text: str = Field(..., min_length=1, max_length=2000)
    image_count: int = Field(1, ge=1, le=12)
    style: ArtStyle = ArtStyle.CARTOON
    llm_credential: Optional[LLMCredential] = None


class StoryboardResponse(BaseModel):
    title: str = Field("", max_length=120)
    parsed_data: ParsedData
    consistency: StoryboardConsistency
    consistency_prompt: str = Field("", max_length=1000)
    panels: List[StoryboardPanel]
    negative_prompt: str = Field("", max_length=500)


class ImageGenerateRequest(BaseModel):
    """图像生成请求（Phase 5，无状态一站式）

    请求方（C++ 后端）负责持有任务状态，这里一次调用完成：
    文本解析 → Prompt 优化 → 图像生成 → 落盘。

    `llm_credential` / `image_credential` 为 BYOK 可选字段：
    有值时按请求构造 Provider，缺失时回退系统默认 Provider。
    """

    text: str = Field(..., min_length=1, max_length=2000, description="用户输入的故事文本")
    style: ArtStyle = Field(default=ArtStyle.CARTOON, description="艺术风格")
    user_id: Optional[str] = Field(None, description="用户 ID（用于存储分层）")
    book_id: Optional[str] = Field(None, description="绘本 ID（用于存储分层）")
    task_id: Optional[str] = Field(None, description="后端任务 ID（仅透传记录，不参与状态管理）")
    parameters: Dict[str, Any] = Field(default_factory=dict, description="生成参数（宽高等）")
    # 批次编排：由 storyboard 阶段生成一次，N 个任务共享
    parsed_data: Optional[ParsedData] = Field(None, description="已解析的故事结构（避免重复 LLM）")
    consistency_prompt: Optional[str] = Field(None, max_length=1000)
    negative_prompt_extra: Optional[str] = Field(None, max_length=500)
    llm_credential: Optional[LLMCredential] = Field(
        None, description="用户自带 LLM 凭据（可选）"
    )
    image_credential: Optional[ImageCredential] = Field(
        None, description="用户自带图像凭据（可选）"
    )


class GeneratedImageInfo(BaseModel):
    """落盘后的生成图信息（Phase 5）"""

    image_id: str
    file_path: str
    width: int
    height: int
    format: str
    file_size: int
    seed: Optional[int] = None
    provider: str = "mock"


class ImageGenerateResponse(BaseModel):
    """图像生成响应（Phase 5）"""

    # `model_used` 与数据库字段保持一致，需关闭 pydantic 的 model_ 保留命名空间校验
    model_config = ConfigDict(protected_namespaces=())

    status: str = "success"
    prompt: str
    negative_prompt: str = ""
    parsed_data: Optional[ParsedData] = None
    model_used: str = ""
    content_review_status: str = "approved"
    content_review_reason: str = ""
    image: GeneratedImageInfo


class ImageGenerationParams(BaseModel):
    """图像生成参数（Provider 层入参）"""

    prompt: str
    negative_prompt: str = ""
    width: int = 1024
    height: int = 1024
    steps: int = 30
    cfg_scale: float = 7.5
    seed: Optional[int] = None
    style_preset: Optional[str] = None


class ImageGenerationResult(BaseModel):
    """图像生成结果（Provider 层出参）"""

    image_url: str
    width: int
    height: int
    seed: Optional[int] = None
    finish_reason: str = "success"
    # Provider 若直接返回二进制（如 Mock），可通过该字段携带，避免二次下载
    image_bytes: Optional[bytes] = None
    content_type: Optional[str] = None
    # 厂商返回的内容过滤 / 审核元数据，仅保存结构化字段，不保存完整响应体
    provider_metadata: Optional[Dict[str, Any]] = None


class AnalysisResult(BaseModel):
    """文本解析结果"""

    parsed_data: ParsedData
    confidence: float = 0.0
    raw_response: str = ""
