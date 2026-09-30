"""Prompt 优化服务（Phase 4）。

纯计算、无 I/O，因此为同步方法。
"""

from __future__ import annotations

from typing import Dict, Optional

from ..models.schemas import ArtStyle, ParsedData
from ..prompts.prompt_optimization import PromptOptimizationPrompt
from ..prompts.styles import get_style_template
from .base import BaseService


class PromptOptimizerService(BaseService):
    """把结构化数据 + 风格模板优化为最终绘图 Prompt。"""

    def __init__(self) -> None:
        super().__init__()
        self.builder = PromptOptimizationPrompt()

    def style_template(self, style: ArtStyle | str) -> Dict[str, object]:
        """取风格模板。"""
        return get_style_template(style)

    def optimize(
        self,
        parsed_data: ParsedData,
        style: ArtStyle | str = ArtStyle.CARTOON,
        additional_params: Optional[Dict[str, object]] = None,
    ) -> str:
        """生成正向 Prompt。"""
        template = self.style_template(style)
        return self.builder.build(parsed_data, template, additional_params)

    def negative(
        self,
        style: ArtStyle | str = ArtStyle.CARTOON,
    ) -> str:
        """生成负向 Prompt。"""
        template = self.style_template(style)
        return self.builder.negative_prompt(template)
