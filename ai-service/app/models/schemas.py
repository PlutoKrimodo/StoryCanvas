from pydantic import BaseModel, Field
from typing import List, Optional, Dict, Any
from enum import Enum


class TaskStatus(str, Enum):
    """任务状态"""
    PENDING = "pending"
    PROCESSING = "processing"
    COMPLETED = "completed"
    FAILED = "failed"


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
    characters: List[ParsedCharacter] = []
    scene: ParsedScene = ParsedScene()
    objects: List[str] = []
    actions: List[str] = []
    emotions: List[str] = []
    style: Dict[str, Any] = {}


class GenerationRequest(BaseModel):
    """生成请求"""
    text: str = Field(..., description="用户输入的故事文本")
    book_id: Optional[str] = Field(None, description="绘本 ID")
    page_id: Optional[str] = Field(None, description="页面 ID")
    character_ids: List[str] = Field(default=[], description="角色 ID 列表")
    style: ArtStyle = Field(default=ArtStyle.CARTOON, description="艺术风格")
    parameters: Dict[str, Any] = Field(default={}, description="生成参数")


class GenerationResponse(BaseModel):
    """生成响应"""
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


class AnalysisResult(BaseModel):
    """分析结果"""
    parsed_data: ParsedData
    confidence: float = 0.0
    raw_response: str = ""
