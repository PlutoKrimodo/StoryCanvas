"""图像生成编排服务（Phase 5，AI 侧一站式）。

一次调用完成：文本解析 → Prompt 优化 → 图像生成 → 落盘。
AI 服务保持**无状态**（任务状态由 C++ 后端持有并落库，见 docs/16 §4.2 决策 3）。
"""

from __future__ import annotations

import math
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
from . import content_policy
from .base import BaseService
from .image_generator import ImageGeneratorService
from .prompt_optimizer import PromptOptimizerService
from .storyboard_service import sanitize_panel_text
from .text_analyzer import TextAnalyzerService

# 尺寸合法区间（防止恶意/异常参数拖垮生成）
_MIN_SIZE = 256
_MAX_SIZE = 2048
# Seedream 5.0 的 1K 档位下限为 1024x1024（1,048,576 像素）；
# 低于该值（如 1280x768 = 983,040）可能返回 400 InvalidParameter，故等比放大兜底。
_MIN_AREA = 1024 * 1024


def _format_size(num_bytes: int) -> str:
    """人类可读的字节数（Q8 失败文案用）。

    上限可经 MAX_IMAGE_SIZE 配置为任意值（联调/验收时常用极小值），
    因此不能一律按 MB 取整——小于 1MB 时退化为 KB / B。
    """
    if num_bytes < 1024:
        return f"{num_bytes}B"
    if num_bytes < 1024 * 1024:
        return f"{num_bytes / 1024:.1f}KB"
    return f"{num_bytes / (1024 * 1024):.1f}MB"


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

        width = clamp(width, 1024)
        height = clamp(height, 1024)

        # 面积兜底：过小的尺寸真实厂商会拒绝（400），按比例放大到合规区间
        if width * height < _MIN_AREA:
            scale = math.sqrt(_MIN_AREA / (width * height)) * 1.02  # 留 2% 余量
            width = min(_MAX_SIZE, int(math.ceil(width * scale / 8) * 8))
            height = min(_MAX_SIZE, int(math.ceil(height * scale / 8) * 8))

        return width, height

    async def generate(
        self,
        request: ImageGenerateRequest,
        parsed_data: Optional[ParsedData] = None,
    ) -> ImageGenerateResponse:
        # 输入预检：批次单格可能被用户编辑成新的不安全描述，因此即使
        # storyboard 阶段已通过，这里仍要在调用 LLM / 图像厂商前再检一次。
        content_policy.assert_allowed(request.text)

        # 1. 文本解析：批次会把 storyboard 阶段的 parsed_data 透传进来，
        #    此时 N 个任务只解析一次，禁止再次调用 LLM。
        if parsed_data is None:
            parsed_data = request.parsed_data
        if parsed_data is None:
            analysis = await self.analyzer.analyze(request.text)
            parsed_data = analysis.parsed_data

        # 2. Prompt 优化
        style = request.style
        # 防御性清洗：上游漏掉的“第X格/整组统一…”在这里再处理一次
        panel_text = sanitize_panel_text(request.text)
        if request.parsed_data is not None:
            # 批次单格：只使用当前格文本 + 角色外观，避免整段故事污染单格
            prompt = self.optimizer.optimize_panel(
                panel_text, parsed_data, style, request.parameters
            )
        else:
            prompt = self.optimizer.optimize(parsed_data, style, request.parameters)
            if panel_text.strip():
                prompt = f"{prompt}, scene: {panel_text.strip()[:500]}"

        width, height = self._resolve_size(request.parameters)
        # 宽幅更容易被理解为拼图/多格，补一个正向提示（不强制改用户尺寸）
        if width > height:
            prompt = f"wide landscape single scene, {prompt}"
        if request.consistency_prompt:
            prompt = f"{request.consistency_prompt}\n{prompt}"

        # 3. 负向 Prompt：风格负向 + 防分屏/多格 + 用户/分镜附加（去重）
        seen_negative = set()
        negative_items = []
        for item in (
            self.optimizer.negative(style),
            self.optimizer.anti_panel_negative(),
            request.negative_prompt_extra or "",
        ):
            for chunk in str(item).split(","):
                value = chunk.strip()
                if value and value.lower() not in seen_negative:
                    seen_negative.add(value.lower())
                    negative_items.append(value)
        negative = ", ".join(negative_items)

        # 4. 图像生成
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

        # 5. 输出复核：先判定是否可进入落盘与绘本，再写文件。
        review_status, review_reason = content_policy.review_output(
            provider_name=self.image_generator.provider_name,
            prompt=prompt,
            negative_prompt=negative,
            finish_reason=result.finish_reason,
            provider_metadata=result.provider_metadata,
        )

        # 6. 二进制化 + 落盘
        data, content_type = await self.image_generator.materialize(result)
        if len(data) > settings.MAX_IMAGE_SIZE:
            # Q8：错误信息直接透传到后端任务 error_message 与前端失败提示，
            # 必须说清「为什么失败、可以怎么办」，避免答辩现场只有「生成失败」
            raise AIServiceException(
                f"生成图片超过大小上限（{_format_size(len(data))} > "
                f"{_format_size(settings.MAX_IMAGE_SIZE)}），已中止保存；请重试或调整参数",
                502,
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
            content_review_status=review_status,
            content_review_reason=review_reason,
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
