"""图像生成编排服务（Phase 5，AI 侧一站式）。

一次调用完成：文本解析 → Prompt 优化 → 图像生成 → 落盘。
AI 服务保持**无状态**（任务状态由 C++ 后端持有并落库，见 docs/16 §4.2 决策 3）。
"""

from __future__ import annotations

from typing import Any, Dict, Optional

from ..models.schemas import (
    GeneratedImageInfo,
    ImageGenerateRequest,
    ImageGenerateResponse,
    ImageGenerationParams,
    ParsedData,
)
from ..utils.config import settings
from ..utils.exceptions import AIServiceException
from ..utils.storage import build_image_rel_path, guess_extension, new_image_id, save_bytes
from .base import BaseService
from .image_generator import ImageGeneratorService
from .prompt_optimizer import PromptOptimizerService
from .text_analyzer import TextAnalyzerService

# 尺寸合法区间（防止恶意/异常参数拖垮生成）
_MIN_SIZE = 256
_MAX_SIZE = 2048


class GenerationService(BaseService):
    """一站式图像生成编排。"""

    def __init__(
        self,
        analyzer: TextAnalyzerService,
        optimizer: PromptOptimizerService,
        image_generator: ImageGeneratorService,
    ) -> None:
        super().__init__()
        self.analyzer = analyzer
        self.optimizer = optimizer
        self.image_generator = image_generator

    @staticmethod
    def _resolve_size(parameters: Dict[str, Any]) -> tuple[int, int]:
        """从 parameters 解析宽高，支持 `width/height` 或 `size: "1024x1024"`。"""
        width = parameters.get("width")
        height = parameters.get("height")

        size = parameters.get("size")
        if isinstance(size, str) and "x" in size.lower():
            parts = size.lower().split("x")
            try:
                width = int(parts[0])
                height = int(parts[1])
            except (ValueError, IndexError):
                pass

        def clamp(value: Any, default: int) -> int:
            try:
                number = int(value)
            except (TypeError, ValueError):
                return default
            return max(_MIN_SIZE, min(number, _MAX_SIZE))

        return clamp(width, 1024), clamp(height, 1024)

    async def generate(
        self,
        request: ImageGenerateRequest,
        parsed_data: Optional[ParsedData] = None,
    ) -> ImageGenerateResponse:
        # 1. 文本解析（如调用方已提供结构化数据则跳过，避免重复消耗 LLM）
        if parsed_data is None:
            analysis = await self.analyzer.analyze(request.text)
            parsed_data = analysis.parsed_data

        # 2. Prompt 优化
        style = request.style
        prompt = self.optimizer.optimize(parsed_data, style, request.parameters)
        negative = self.optimizer.negative(style)

        # 3. 图像生成
        width, height = self._resolve_size(request.parameters)
        seed = request.parameters.get("seed")
        params = ImageGenerationParams(
            prompt=prompt,
            negative_prompt=negative,
            width=width,
            height=height,
            seed=int(seed) if isinstance(seed, (int, float)) else None,
            style_preset=style.value if hasattr(style, "value") else str(style),
        )
        result = await self.image_generator.generate(params)

        # 4. 二进制化 + 落盘
        data, content_type = await self.image_generator.materialize(result)
        if len(data) > settings.MAX_IMAGE_SIZE:
            raise AIServiceException(
                f"生成图超过大小上限（{len(data)} > {settings.MAX_IMAGE_SIZE}）", 502
            )

        image_id = new_image_id()
        extension = guess_extension(content_type, result.image_url)
        rel_path = build_image_rel_path(request.user_id, request.book_id, image_id, extension)
        file_size = save_bytes(rel_path, data)

        return ImageGenerateResponse(
            status="success",
            prompt=prompt,
            negative_prompt=negative,
            parsed_data=parsed_data,
            model_used=self.image_generator.provider_name,
            image=GeneratedImageInfo(
                image_id=image_id,
                file_path=rel_path,
                width=result.width,
                height=result.height,
                format=extension,
                file_size=file_size,
                seed=result.seed,
                provider=self.image_generator.provider_name,
            ),
        )
