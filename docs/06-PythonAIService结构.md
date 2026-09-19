# Python AI Service 结构设计

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

**版本**: 1.0  
**日期**: 2026-09-17  
**作者**: StoryCanvas 开发团队

---

## 1. 项目目录结构

### 1.1 完整目录结构

```
ai-service/
├── app/
│   ├── __init__.py
│   ├── main.py                     # FastAPI 应用入口
│   ├── api/                        # API 路由层
│   │   ├── __init__.py
│   │   ├── v1/                     # API 版本
│   │   │   ├── __init__.py
│   │   │   ├── router.py           # 路由注册
│   │   │   ├── generation.py       # 图像生成 API
│   │   │   ├── analysis.py         # 文本分析 API
│   │   │   ├── health.py           # 健康检查 API
│   │   │   └── dependencies.py     # 依赖注入
│   │   └── deps.py                 # 通用依赖
│   ├── services/                   # 业务逻辑层
│   │   ├── __init__.py
│   │   ├── text_analyzer.py        # 文本分析服务
│   │   ├── prompt_optimizer.py     # Prompt 优化服务
│   │   ├── image_generator.py      # 图像生成服务
│   │   ├── task_manager.py         # 任务管理服务
│   │   └── base.py                 # 服务基类
│   ├── providers/                  # AI 服务提供商
│   │   ├── __init__.py
│   │   ├── base.py                 # 提供商基类
│   │   ├── llm/                    # LLM 提供商
│   │   │   ├── __init__.py
│   │   │   ├── base.py             # LLM 基类
│   │   │   ├── openai_provider.py  # OpenAI 实现
│   │   │   ├── zhipu_provider.py   # 智谱 AI 实现
│   │   │   └── mock_provider.py    # Mock 提供商（测试用）
│   │   └── image/                  # 图像生成提供商
│   │       ├── __init__.py
│   │       ├── base.py             # 图像生成基类
│   │       ├── dalle_provider.py   # DALL-E 实现
│   │       ├── stability_provider.py # Stability AI 实现
│   │       ├── sdwebui_provider.py # Stable Diffusion WebUI
│   │       └── mock_provider.py    # Mock 提供商（测试用）
│   ├── models/                     # 数据模型
│   │   ├── __init__.py
│   │   ├── schemas.py              # Pydantic 模型
│   │   ├── enums.py                # 枚举类型
│   │   └── database.py             # 数据库模型
│   ├── prompts/                    # Prompt 模板
│   │   ├── __init__.py
│   │   ├── text_analysis.py        # 文本分析 Prompt
│   │   ├── prompt_optimization.py  # Prompt 优化
│   │   └── styles/                 # 风格模板
│   │       ├── __init__.py
│   │       ├── cartoon.py          # 卡通风格
│   │       ├── watercolor.py       # 水彩风格
│   │       ├── oil_painting.py     # 油画风格
│   │       └── anime.py            # 动漫风格
│   └── utils/                      # 工具函数
│       ├── __init__.py
│       ├── config.py               # 配置管理
│       ├── logger.py               # 日志管理
│       ├── exceptions.py           # 异常定义
│       ├── http_client.py          # HTTP 客户端
│       └── validators.py           # 验证工具
├── tests/                          # 测试
│   ├── __init__.py
│   ├── conftest.py                 # 测试配置
│   ├── test_services/              # 服务测试
│   │   ├── __init__.py
│   │   ├── test_text_analyzer.py
│   │   ├── test_prompt_optimizer.py
│   │   └── test_image_generator.py
│   ├── test_providers/             # 提供商测试
│   │   ├── __init__.py
│   │   ├── test_openai_provider.py
│   │   └── test_mock_provider.py
│   └── test_api/                   # API 测试
│       ├── __init__.py
│       └── test_generation.py
├── prompts/                        # Prompt 模板文件
│   ├── text_analysis.txt
│   ├── prompt_optimization.txt
│   └── style_templates/
│       ├── cartoon.txt
│       ├── watercolor.txt
│       └── anime.txt
├── requirements.txt                # Python 依赖
├── requirements-dev.txt            # 开发依赖
├── pyproject.toml                  # 项目配置
├── .env.example                    # 环境变量示例
├── Dockerfile                      # Docker 配置
├── README.md                       # 项目说明
└── main.py                         # 入口文件
```

---

## 2. 核心模块设计

### 2.1 应用入口

#### main.py

```python
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager

from app.api.v1.router import api_router
from app.utils.config import settings
from app.utils.logger import setup_logging

@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理"""
    # 启动时
    setup_logging()
    yield
    # 关闭时
    pass

def create_app() -> FastAPI:
    """创建 FastAPI 应用"""
    app = FastAPI(
        title="StoryCanvas AI Service",
        description="基于生成式 AI 的儿童绘本智能创作服务",
        version="1.0.0",
        lifespan=lifespan
    )
    
    # 配置 CORS
    app.add_middleware(
        CORSMiddleware,
        allow_origins=settings.CORS_ORIGINS,
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )
    
    # 注册路由
    app.include_router(api_router, prefix="/api/v1")
    
    return app

app = create_app()
```

#### main.py (根目录)

```python
import uvicorn
from app.utils.config import settings

if __name__ == "__main__":
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=settings.DEBUG,
        workers=settings.WORKERS
    )
```

### 2.2 配置管理

#### utils/config.py

```python
from pydantic import BaseSettings
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
    MAX_IMAGE_SIZE: int = 10 * 1024 * 1024  # 10MB
    
    # 任务配置
    MAX_CONCURRENT_TASKS: int = 5
    TASK_RETRY_COUNT: int = 3
    
    # 内部 API 配置
    INTERNAL_API_KEY: str = ""
    
    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"

settings = Settings()
```

### 2.3 数据模型

#### models/schemas.py

```python
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
    parsed_data: Optional[ParsedData]
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
```

### 2.4 服务层

#### services/base.py

```python
from abc import ABC, abstractmethod
from typing import Any, Dict
import logging

class BaseService(ABC):
    """服务基类"""
    
    def __init__(self):
        self.logger = logging.getLogger(self.__class__.__name__)
    
    @abstractmethod
    async def execute(self, *args, **kwargs) -> Any:
        """执行服务逻辑"""
        pass
    
    async def validate_input(self, data: Dict[str, Any]) -> bool:
        """验证输入数据"""
        return True
```

#### services/text_analyzer.py

```python
from typing import Optional
from .base import BaseService
from ..models.schemas import ParsedData, AnalysisResult
from ..providers.llm.base import LLMProvider
from ..prompts.text_analysis import TextAnalysisPrompt

class TextAnalyzerService(BaseService):
    """文本分析服务"""
    
    def __init__(self, llm_provider: LLMProvider):
        super().__init__()
        self.llm_provider = llm_provider
        self.prompt_builder = TextAnalysisPrompt()
    
    async def analyze(self, text: str) -> AnalysisResult:
        """分析文本"""
        try:
            # 1. 构建分析 Prompt
            prompt = self.prompt_builder.build_prompt(text)
            
            # 2. 调用 LLM
            response = await self.llm_provider.generate(
                messages=prompt,
                temperature=0.3,
                max_tokens=1000
            )
            
            # 3. 解析响应
            parsed_data = self._parse_response(response)
            
            return AnalysisResult(
                parsed_data=parsed_data,
                confidence=0.9,
                raw_response=response
            )
            
        except Exception as e:
            self.logger.error(f"文本分析失败: {e}")
            raise
    
    def _parse_response(self, response: str) -> ParsedData:
        """解析 LLM 响应"""
        # 实现 JSON 解析逻辑
        import json
        try:
            # 尝试从响应中提取 JSON
            start = response.find('{')
            end = response.rfind('}') + 1
            if start != -1 and end != -1:
                json_str = response[start:end]
                data = json.loads(json_str)
                return ParsedData(**data)
        except json.JSONDecodeError:
            pass
        
        # 返回默认解析结果
        return ParsedData()
```

#### services/prompt_optimizer.py

```python
from typing import Optional
from .base import BaseService
from ..models.schemas import ParsedData, ArtStyle
from ..prompts.prompt_optimization import PromptOptimizationPrompt

class PromptOptimizerService(BaseService):
    """Prompt 优化服务"""
    
    def __init__(self):
        super().__init__()
        self.prompt_builder = PromptOptimizationPrompt()
        self.style_templates = {}
    
    async def optimize(
        self,
        parsed_data: ParsedData,
        style: ArtStyle,
        additional_params: dict = None
    ) -> str:
        """优化 Prompt"""
        try:
            # 1. 获取风格模板
            style_template = self._get_style_template(style)
            
            # 2. 构建优化 Prompt
            prompt = self.prompt_builder.build_prompt(
                parsed_data=parsed_data,
                style_template=style_template,
                additional_params=additional_params
            )
            
            # 3. 返回优化后的 Prompt
            return self._format_final_prompt(parsed_data, style_template)
            
        except Exception as e:
            self.logger.error(f"Prompt 优化失败: {e}")
            raise
    
    def _get_style_template(self, style: ArtStyle) -> dict:
        """获取风格模板"""
        templates = {
            ArtStyle.CARTOON: {
                "keywords": ["cartoon", "colorful", "playful", "children's book"],
                "quality": "high quality, detailed"
            },
            ArtStyle.WATERCOLOR: {
                "keywords": ["watercolor painting", "soft colors", "gentle"],
                "quality": "artistic, beautiful"
            },
            # ... 其他风格
        }
        return templates.get(style, templates[ArtStyle.CARTOON])
    
    def _format_final_prompt(
        self,
        parsed_data: ParsedData,
        style_template: dict
    ) -> str:
        """格式化最终 Prompt"""
        parts = []
        
        # 添加风格关键词
        parts.extend(style_template.get("keywords", []))
        
        # 添加场景描述
        if parsed_data.scene:
            scene = parsed_data.scene
            if scene.location:
                parts.append(scene.location)
            if scene.mood:
                parts.append(scene.mood)
        
        # 添加角色描述
        for char in parsed_data.characters:
            if char.appearance:
                parts.append(char.appearance)
        
        # 添加动作
        parts.extend(parsed_data.actions[:2])
        
        # 添加质量描述
        parts.append(style_template.get("quality", ""))
        
        return ", ".join(filter(None, parts))
```

#### services/image_generator.py

```python
from typing import Optional
import asyncio
from .base import BaseService
from ..models.schemas import (
    ImageGenerationParams,
    ImageGenerationResult,
    TaskStatus
)
from ..providers.image.base import ImageProvider

class ImageGeneratorService(BaseService):
    """图像生成服务"""
    
    def __init__(self, image_provider: ImageProvider):
        super().__init__()
        self.image_provider = image_provider
        self.max_retries = 3
        self.timeout = 120
    
    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        """生成图像"""
        last_error = None
        
        for attempt in range(self.max_retries):
            try:
                # 调用图像生成提供商
                result = await asyncio.wait_for(
                    self.image_provider.generate(params),
                    timeout=self.timeout
                )
                return result
                
            except asyncio.TimeoutError:
                last_error = f"图像生成超时 (尝试 {attempt + 1}/{self.max_retries})"
                self.logger.warning(last_error)
                
            except Exception as e:
                last_error = f"图像生成失败: {e} (尝试 {attempt + 1}/{self.max_retries})"
                self.logger.error(last_error)
            
            # 等待后重试
            if attempt < self.max_retries - 1:
                await asyncio.sleep(2 ** attempt)
        
        raise Exception(f"图像生成最终失败: {last_error}")
```

#### services/task_manager.py

```python
from typing import Optional, Dict, Any
import asyncio
from datetime import datetime
import uuid
from .base import BaseService
from .text_analyzer import TextAnalyzerService
from .prompt_optimizer import PromptOptimizerService
from .image_generator import ImageGeneratorService
from ..models.schemas import (
    GenerationRequest,
    TaskResult,
    TaskStatus,
    ParsedData
)

class TaskManagerService(BaseService):
    """任务管理服务"""
    
    def __init__(
        self,
        text_analyzer: TextAnalyzerService,
        prompt_optimizer: PromptOptimizerService,
        image_generator: ImageGeneratorService
    ):
        super().__init__()
        self.text_analyzer = text_analyzer
        self.prompt_optimizer = prompt_optimizer
        self.image_generator = image_generator
        self.tasks: Dict[str, TaskResult] = {}
    
    async def create_task(self, request: GenerationRequest) -> str:
        """创建生成任务"""
        task_id = str(uuid.uuid4())
        
        # 创建初始任务记录
        task = TaskResult(
            task_id=task_id,
            status=TaskStatus.PENDING,
            prompt="",
            original_text=request.text,
            parsed_data=None,
            created_at=datetime.utcnow().isoformat()
        )
        self.tasks[task_id] = task
        
        # 异步执行任务
        asyncio.create_task(self._execute_task(task_id, request))
        
        return task_id
    
    async def _execute_task(self, task_id: str, request: GenerationRequest):
        """执行生成任务"""
        try:
            # 1. 更新状态为处理中
            self._update_task_status(task_id, TaskStatus.PROCESSING)
            
            # 2. 文本分析
            analysis_result = await self.text_analyzer.analyze(request.text)
            
            # 3. Prompt 优化
            prompt = await self.prompt_optimizer.optimize(
                parsed_data=analysis_result.parsed_data,
                style=request.style,
                additional_params=request.parameters
            )
            
            # 4. 生成图像
            from ..models.schemas import ImageGenerationParams
            image_params = ImageGenerationParams(
                prompt=prompt,
                **request.parameters
            )
            image_result = await self.image_generator.generate(image_params)
            
            # 5. 更新任务为完成
            self._complete_task(
                task_id=task_id,
                prompt=prompt,
                parsed_data=analysis_result.parsed_data,
                result={
                    "image_url": image_result.image_url,
                    "width": image_result.width,
                    "height": image_result.height
                }
            )
            
        except Exception as e:
            # 6. 更新任务为失败
            self._fail_task(task_id, str(e))
    
    def get_task_status(self, task_id: str) -> Optional[TaskResult]:
        """获取任务状态"""
        return self.tasks.get(task_id)
    
    def _update_task_status(self, task_id: str, status: TaskStatus):
        """更新任务状态"""
        if task_id in self.tasks:
            self.tasks[task_id].status = status
    
    def _complete_task(
        self,
        task_id: str,
        prompt: str,
        parsed_data: ParsedData,
        result: Dict[str, Any]
    ):
        """完成任务"""
        if task_id in self.tasks:
            task = self.tasks[task_id]
            task.status = TaskStatus.COMPLETED
            task.prompt = prompt
            task.parsed_data = parsed_data
            task.result = result
            task.completed_at = datetime.utcnow().isoformat()
    
    def _fail_task(self, task_id: str, error_message: str):
        """任务失败"""
        if task_id in self.tasks:
            task = self.tasks[task_id]
            task.status = TaskStatus.FAILED
            task.error_message = error_message
            task.completed_at = datetime.utcnow().isoformat()
```

### 2.5 Provider 层

#### providers/image/base.py

```python
from abc import ABC, abstractmethod
from typing import Optional
import logging

class ImageProvider(ABC):
    """图像生成提供商基类"""
    
    def __init__(self):
        self.logger = logging.getLogger(self.__class__.__name__)
    
    @abstractmethod
    async def generate(self, params) -> 'ImageGenerationResult':
        """生成图像"""
        pass
    
    @abstractmethod
    async def health_check(self) -> bool:
        """健康检查"""
        pass
```

#### providers/image/mock_provider.py

```python
import asyncio
import random
from .base import ImageProvider
from ...models.schemas import ImageGenerationParams, ImageGenerationResult

class MockImageProvider(ImageProvider):
    """Mock 图像生成提供商（用于测试）"""
    
    def __init__(self):
        super().__init__()
        self.generation_delay = 2  # 模拟生成延迟
    
    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        """模拟生成图像"""
        # 模拟处理延迟
        await asyncio.sleep(self.generation_delay)
        
        # 模拟偶尔失败
        if random.random() < 0.1:  # 10% 失败率
            raise Exception("Mock: 图像生成失败")
        
        # 返回模拟结果
        return ImageGenerationResult(
            image_url="https://picsum.photos/1024/1024",
            width=params.width,
            height=params.height,
            seed=params.seed or random.randint(0, 1000000),
            finish_reason="success"
        )
    
    async def health_check(self) -> bool:
        """健康检查"""
        return True
```

#### providers/image/dalle_provider.py

```python
import httpx
from typing import Optional
from .base import ImageProvider
from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings

class DallEProvider(ImageProvider):
    """OpenAI DALL-E 提供商"""
    
    def __init__(self):
        super().__init__()
        self.api_key = settings.OPENAI_API_KEY
        self.api_base = settings.OPENAI_API_BASE
        self.model = "dall-e-3"
    
    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        """调用 DALL-E 生成图像"""
        async with httpx.AsyncClient() as client:
            response = await client.post(
                f"{self.api_base}/images/generations",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json"
                },
                json={
                    "model": self.model,
                    "prompt": params.prompt,
                    "n": 1,
                    "size": f"{params.width}x{params.height}",
                    "quality": "standard"
                },
                timeout=settings.IMAGE_TIMEOUT
            )
            
            if response.status_code != 200:
                raise Exception(f"DALL-E API 错误: {response.text}")
            
            data = response.json()
            return ImageGenerationResult(
                image_url=data["data"][0]["url"],
                width=params.width,
                height=params.height,
                finish_reason="success"
            )
    
    async def health_check(self) -> bool:
        """健康检查"""
        try:
            async with httpx.AsyncClient() as client:
                response = await client.get(
                    f"{self.api_base}/models",
                    headers={"Authorization": f"Bearer {self.api_key}"}
                )
                return response.status_code == 200
        except Exception:
            return False
```

### 2.6 API 层

#### api/v1/router.py

```python
from fastapi import APIRouter
from . import generation, analysis, health

api_router = APIRouter()

# 注册路由
api_router.include_router(
    health.router,
    prefix="/health",
    tags=["Health"]
)

api_router.include_router(
    generation.router,
    prefix="/generation",
    tags=["Generation"]
)

api_router.include_router(
    analysis.router,
    prefix="/analysis",
    tags=["Analysis"]
)
```

#### api/v1/generation.py

```python
from fastapi import APIRouter, Depends, HTTPException
from typing import Optional
from ...models.schemas import (
    GenerationRequest,
    GenerationResponse,
    TaskResult,
    TaskStatus
)
from ...services.task_manager import TaskManagerService
from .deps import get_task_manager

router = APIRouter()

@router.post("/", response_model=GenerationResponse)
async def create_generation_task(
    request: GenerationRequest,
    task_manager: TaskManagerService = Depends(get_task_manager)
):
    """创建图像生成任务"""
    try:
        task_id = await task_manager.create_task(request)
        return GenerationResponse(
            task_id=task_id,
            status=TaskStatus.PENDING,
            created_at="now"
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@router.get("/{task_id}", response_model=TaskResult)
async def get_task_status(
    task_id: str,
    task_manager: TaskManagerService = Depends(get_task_manager)
):
    """获取任务状态"""
    task = task_manager.get_task_status(task_id)
    if not task:
        raise HTTPException(status_code=404, detail="任务不存在")
    return task
```

#### api/v1/deps.py

```python
from functools import lru_cache
from ...services.task_manager import TaskManagerService
from ...services.text_analyzer import TextAnalyzerService
from ...services.prompt_optimizer import PromptOptimizerService
from ...services.image_generator import ImageGeneratorService
from ...providers.llm.openai_provider import OpenAIProvider
from ...providers.image.dalle_provider import DallEProvider

@lru_cache()
def get_task_manager() -> TaskManagerService:
    """获取任务管理器"""
    llm_provider = OpenAIProvider()
    image_provider = DallEProvider()
    
    text_analyzer = TextAnalyzerService(llm_provider)
    prompt_optimizer = PromptOptimizerService()
    image_generator = ImageGeneratorService(image_provider)
    
    return TaskManagerService(
        text_analyzer=text_analyzer,
        prompt_optimizer=prompt_optimizer,
        image_generator=image_generator
    )
```

---

## 3. Prompt 模板管理

### 3.1 文本分析 Prompt

#### prompts/text_analysis.py

```python
class TextAnalysisPrompt:
    """文本分析 Prompt 构建器"""
    
    SYSTEM_PROMPT = """你是一个专业的儿童绘本创作助手。请分析用户输入的故事文本，提取以下信息：
1. 人物角色（名称、外观描述、位置）
2. 场景（地点、时间、天气、氛围）
3. 物品
4. 动作
5. 情感
6. 风格建议

请以 JSON 格式返回分析结果。"""
    
    def build_prompt(self, text: str) -> list:
        """构建分析 Prompt"""
        return [
            {"role": "system", "content": self.SYSTEM_PROMPT},
            {"role": "user", "content": f"请分析以下故事文本：\n\n{text}"}
        ]
```

### 3.2 风格模板

#### prompts/styles/cartoon.py

```python
CARTOON_STYLE = {
    "name": "卡通风格",
    "description": "色彩鲜艳、活泼可爱的卡通风格",
    "keywords": [
        "cartoon style",
        "colorful",
        "playful",
        "children's book illustration",
        "cute characters",
        "vibrant colors"
    ],
    "quality": "high quality, detailed, professional illustration",
    "negative_prompt": "realistic, dark, scary, violent"
}
```

---

## 4. 异常处理

#### utils/exceptions.py

```python
class AIServiceException(Exception):
    """AI 服务基础异常"""
    def __init__(self, message: str, code: int = 500):
        self.message = message
        self.code = code
        super().__init__(self.message)

class LLMProviderException(AIServiceException):
    """LLM 提供商异常"""
    pass

class ImageProviderException(AIServiceException):
    """图像生成提供商异常"""
    pass

class TaskTimeoutException(AIServiceException):
    """任务超时异常"""
    def __init__(self, task_id: str):
        super().__init__(f"任务 {task_id} 执行超时", 408)
```

---

## 5. 依赖文件

### 5.1 requirements.txt

```
# Web 框架
fastapi==0.104.1
uvicorn[standard]==0.24.0

# 验证
pydantic==2.5.2
pydantic-settings==2.1.0

# HTTP 客户端
httpx==0.25.2

# OpenAI
openai==1.3.7

# 工具
python-dotenv==1.0.0
python-multipart==0.0.6

# 日志
loguru==0.7.2

# 测试
pytest==7.4.3
pytest-asyncio==0.21.1
pytest-cov==4.1.0
```

### 5.2 .env.example

```bash
# 应用配置
HOST=0.0.0.0
PORT=8000
DEBUG=true

# OpenAI 配置
OPENAI_API_KEY=sk-your-api-key
OPENAI_API_BASE=https://api.openai.com/v1

# Stability AI 配置
STABILITY_API_KEY=sk-your-stability-key

# 内部 API 配置
INTERNAL_API_KEY=your-internal-key

# 存储配置
STORAGE_PATH=./storage
```

---

## 6. 设计原则

### 6.1 Provider 抽象
- 定义统一的接口
- 支持多个 AI 服务商
- 便于测试和替换

### 6.2 依赖注入
- 使用 FastAPI 的依赖注入
- 便于测试和配置

### 6.3 异步处理
- 使用 asyncio 实现异步
- 支持并发处理多个任务

### 6.4 错误处理
- 完善的异常处理机制
- 重试机制
- 超时控制

---

## 7. 扩展性考虑

### 7.1 添加新的 AI 提供商
1. 在 `providers/` 下创建新目录
2. 实现基类接口
3. 在配置中添加新的提供商配置
4. 在依赖注入中注册

### 7.2 添加新的艺术风格
1. 在 `prompts/styles/` 下创建新风格文件
2. 在 `ArtStyle` 枚举中添加新风格
3. 在 `PromptOptimizerService` 中添加风格处理

---

**文档结束**
