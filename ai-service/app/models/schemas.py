from enum import Enum
from typing import Any, Dict, List, Optional

from pydantic import BaseModel, Field


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


class GenerationRequest(BaseModel):
    """生成请求"""

    text: str = Field(..., min_length=1, max_length=2000, description="用户输入的故事文本")
    book_id: Optional[str] = Field(None, description="绘本 ID")
    page_id: Optional[str] = Field(None, description="页容器 ID")
    # 角色一致性属第二阶段能力，本轮不使用；保留字段以便后续扩展
    character_ids: List[str] = Field(
        default_factory=list, description="角色 ID 列表（第二阶段，本轮未启用）"
    )
    style: ArtStyle = Field(default=ArtStyle.CARTOON, description="艺术风格")
    parameters: Dict[str, Any] = Field(default_factory=dict, description="生成参数")


class TextAnalysisRequest(BaseModel):
    """文本解析请求"""

    text: str = Field(..., min_length=1, max_length=2000, description="待解析的故事文本")


class AnalysisResult(BaseModel):
    """文本解析结果"""

    parsed_data: ParsedData
    confidence: float = 0.0
    raw_response: str = ""


class GenerationResponse(BaseModel):
    """生成任务响应"""

    task_id: str
    status: TaskStatus
    created_at: str


class TaskResult(BaseModel):
    """任务结果"""

    task_id: str
    status: TaskStatus
    prompt: str
    original_text: str
    parsed_data: Optional[ParsedData] = None
    result: Optional[Dict[str, Any]] = None
    error_message: Optional[str] = None
    created_at: str
    completed_at: Optional[str] = None


class ImageGenerationParams(BaseModel):
    """图像生成参数"""

    prompt: str
    negative_prompt: str = ""
    width: int = 1024
    height: int = 1024
    steps: int = 30
    cfg_scale: float = 7.5
    seed: Optional[int] = None
    style_preset: Optional[str] = None


class ImageGenerationResult(BaseModel):
    """图像生成结果"""

    image_url: str
    width: int
    height: int
    seed: Optional[int] = None
    finish_reason: str = "success"
