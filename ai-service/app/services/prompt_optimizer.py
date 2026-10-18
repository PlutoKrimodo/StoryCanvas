"""Prompt 优化服务（Phase 4）。

纯计算、无 I/O，因此为同步方法。
"""

from __future__ import annotations

from typing import Dict, List, Optional

from ..models.schemas import ArtStyle, ParsedData
from ..prompts.prompt_optimization import (
    ANTI_PANEL_NEGATIVE,
    SINGLE_IMAGE_CONSTRAINTS,
    PromptOptimizationPrompt,
)
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

    def optimize_panel(
        self,
        panel_text: str,
        parsed_data: ParsedData,
        style: ArtStyle | str = ArtStyle.CARTOON,
        additional_params: Optional[Dict[str, object]] = None,
    ) -> str:
        """生成单格正向 Prompt。

        与 ``optimize`` 的关键差异：**不**使用整段故事的 scene / actions /
        objects / emotions，只保留「当前格文本 + 角色外观」。否则多格故事里的
        其它场景与角色会被塞进同一张图，诱发多人物、多场景的分屏伪影。
        """
        template = self.style_template(style)
        parts: List[str] = list(template.get("keywords", []))  # type: ignore[arg-type]
        parts.append(SINGLE_IMAGE_CONSTRAINTS)
        if panel_text.strip():
            parts.append(f"scene: {panel_text.strip()}")

        # 角色一致性：按 name 去重，只保留外观描述
        seen_names = set()
        for character in parsed_data.characters:
            name = (character.name or "").strip()
            if not name or name in seen_names:
                continue
            seen_names.add(name)
            chunks = [name]
            if character.appearance:
                chunks.append(character.appearance)
            parts.append("character design (keep consistent): " + ", ".join(chunks))

        if additional_params:
            extra = additional_params.get("extra_keywords")
            if isinstance(extra, str) and extra.strip():
                parts.append(extra.strip())

        quality = template.get("quality")
        if quality:
            parts.append(str(quality))

        # 去空、去重（保序）
        seen_parts = set()
        cleaned: List[str] = []
        for item in parts:
            value = str(item).strip().strip(",")
            if value and value not in seen_parts:
                seen_parts.add(value)
                cleaned.append(value)
        return ", ".join(cleaned)

    def negative(
        self,
        style: ArtStyle | str = ArtStyle.CARTOON,
    ) -> str:
        """生成负向 Prompt。"""
        template = self.style_template(style)
        return self.builder.negative_prompt(template)

    @staticmethod
    def anti_panel_negative() -> str:
        """所有风格共用的防分屏/多格负向词。"""
        return ANTI_PANEL_NEGATIVE
