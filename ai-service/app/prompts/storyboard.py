"""分镜规划 Prompt（Storyboard Chat）。

一次 LLM 调用同时产出：
  1. 结构化故事解析（供 Prompt 优化复用，避免 N 张图重复解析）；
  2. N 格分镜描述；
  3. 整组共享的角色 / 配色 / 画风约束；
  4. 负向提示。

要求严格 JSON，便于服务层做代码块 / 首尾大括号三级抽取。
"""

from __future__ import annotations

import re
from typing import List

from ..models.schemas import ArtStyle

_SENTENCE_SPLIT = re.compile(r"[。！？!?；;\n]+")


class StoryboardPrompt:
    """分镜提示词构建与确定性兜底切分。"""

    def build_prompt(self, text: str, image_count: int, style: ArtStyle | str) -> List[dict]:
        style_value = style.value if hasattr(style, "value") else str(style)
        system = (
            "你是儿童绘本的分镜导演。请把用户的故事拆成恰好 {count} 个连续画面，"
            "并保持角色外观、服装、配色与画风一致。\n"
            "只输出 JSON，不要 Markdown 代码块，不要解释。JSON 结构必须为：\n"
            "{{\n"
            '  "title": "不超过 20 字的标题",\n'
            '  "parsed_data": {{\n'
            '    "characters": [{{"name": "", "description": "", "appearance": "", "position": ""}}],\n'
            '    "scene": {{"location": "", "time": "", "weather": "", "mood": ""}},\n'
            '    "objects": [], "actions": [], "emotions": [], "style": {{}}\n'
            "  }},\n"
            '  "consistency": {{\n'
            '    "characters": [{{"name": "", "appearance": "发型/服装/配色等固定特征"}}],\n'
            '    "palette": "整组图的色彩倾向",\n'
            '    "notes": "画风与镜头连续性要求"\n'
            "  }},\n"
            '  "panels": [{{"index": 0, "text": "第 1 格画面描述"}}],\n'
            '  "negative_prompt": "不希望出现的元素"\n'
            "}}\n"
            "硬性要求：\n"
            "- panels 必须恰好 {count} 个，index 从 0 连续递增；\n"
            "- 每格 text 是可直接绘制的画面描述（主体 + 动作 + 场景），不要写“同上”；\n"
            "- 角色外观必须在 consistency.characters 中固定写明，供每格复用；\n"
            "- 风格为 {style}，所有画面保持同一画风；\n"
            "- 故事较短时补充合理细节，但不得改变原意。"
        ).format(count=image_count, style=style_value)

        user = f"故事原文：\n{text}\n\n请拆成 {image_count} 格，输出 JSON。"

        return [
            {"role": "system", "content": system},
            {"role": "user", "content": user},
        ]

    @staticmethod
    def split_fallback(text: str, image_count: int) -> List[str]:
        """LLM 失败时的确定性切分：按句号/问号/分号/换行切，再合并或补齐。"""
        chunks = [item.strip() for item in _SENTENCE_SPLIT.split(text or "") if item.strip()]
        if not chunks:
            chunks = [(text or "故事画面").strip() or "故事画面"]

        if len(chunks) >= image_count:
            # 均匀合并相邻句子，避免第一格过长、最后一格过短
            merged: List[str] = []
            for index in range(image_count):
                start = round(index * len(chunks) / image_count)
                end = round((index + 1) * len(chunks) / image_count)
                segment = "，".join(chunks[start:max(end, start + 1)])
                merged.append(segment)
            return [item[:500] for item in merged]

        panels = list(chunks)
        while len(panels) < image_count:
            panels.append(f"延续上一格的场景与角色，画面第 {len(panels) + 1} 格：{chunks[-1]}")
        return [item[:500] for item in panels[:image_count]]
