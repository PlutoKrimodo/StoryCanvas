"""Prompt 优化构建器（Phase 4）。

把结构化解析结果 + 风格模板拼装为**英文绘图 Prompt**：
主流图像模型（Seedream / 通义万相 / SD 系）对英文 Prompt 的遵循度更好，
因此这里统一输出英文，同时保留中文实体（角色名、地点）以免语义丢失。
"""

from __future__ import annotations

from typing import Dict, List, Optional

from ..models.schemas import ParsedData


class PromptOptimizationPrompt:
    """绘图 Prompt 构建器。"""

    def build(
        self,
        parsed_data: ParsedData,
        style_template: Dict[str, object],
        additional_params: Optional[Dict[str, object]] = None,
    ) -> str:
        """按「风格 → 场景 → 角色 → 动作 → 物品 → 情绪 → 质量」顺序拼装。"""
        parts: List[str] = []

        # 1. 风格关键词
        parts.extend(style_template.get("keywords", []))  # type: ignore[arg-type]

        scene = parsed_data.scene
        if scene.location:
            parts.append(f"setting: {scene.location}")
        if scene.time:
            parts.append(f"time: {scene.time}")
        if scene.weather:
            parts.append(f"weather: {scene.weather}")
        if scene.mood:
            parts.append(f"atmosphere: {scene.mood}")

        # 2. 角色（含外观，外观是角色一致性的关键）
        for character in parsed_data.characters:
            chunks = [character.name]
            if character.appearance:
                chunks.append(character.appearance)
            if character.position:
                chunks.append(f"positioned {character.position}")
            parts.append(", ".join(chunks))

        # 3. 动作 / 物品 / 情绪
        parts.extend(parsed_data.actions[:3])
        parts.extend(parsed_data.objects[:5])
        parts.extend(parsed_data.emotions[:3])

        # 4. 附加参数（如用户补充的关键词）
        if additional_params:
            extra = additional_params.get("extra_keywords")
            if isinstance(extra, str) and extra.strip():
                parts.append(extra.strip())

        # 5. 质量描述收尾
        quality = style_template.get("quality")
        if quality:
            parts.append(str(quality))

        # 去空、去重（保序）
        seen = set()
        cleaned: List[str] = []
        for item in parts:
            value = str(item).strip().strip(",")
            if value and value not in seen:
                seen.add(value)
                cleaned.append(value)

        return ", ".join(cleaned)

    @staticmethod
    def negative_prompt(style_template: Dict[str, object]) -> str:
        """取风格的负向 Prompt。"""
        return str(style_template.get("negative_prompt", ""))
