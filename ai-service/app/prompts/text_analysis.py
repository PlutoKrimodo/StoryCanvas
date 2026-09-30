"""文本分析 Prompt 构建器（Phase 4）。"""

from __future__ import annotations

import json
from typing import Dict, List

from ..models.schemas import ParsedData

# DeepSeek 的 JSON 模式要求提示词中必须出现 "json" 字样
SYSTEM_PROMPT = """你是一名专业的儿童绘本创作助手。请分析用户给出的故事文本，
抽取可用于插画创作的结构化信息，并**严格以 json 对象**返回（不要输出任何解释、不要使用 markdown 代码块）。

返回的 json 结构必须如下：
{
  "characters": [
    {"name": "角色名", "description": "角色简要描述", "appearance": "外观（如服饰、发色、体型）", "position": "在画面中的位置"}
  ],
  "scene": {"location": "地点", "time": "时间", "weather": "天气", "mood": "氛围"},
  "objects": ["画面中出现的物品"],
  "actions": ["角色正在做的动作"],
  "emotions": ["情绪关键词"],
  "style": {"suggested": "建议的艺术风格关键词"}
}

要求：
1. 只依据用户文本推断，不要编造与文本无关的信息；
2. 无法判断的字段使用空字符串或空数组；
3. characters 最多 3 个，objects / actions / emotions 各最多 5 个；
4. 所有文本使用简洁的中文短语。"""


class TextAnalysisPrompt:
    """文本分析 Prompt 构建器。"""

    def build_prompt(self, text: str) -> List[Dict[str, str]]:
        """构造 chat 消息列表。"""
        return [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": f"请分析以下故事文本，并按要求返回 json：\n\n{text}"},
        ]

    @staticmethod
    def fallback(text: str) -> ParsedData:
        """模型不可用时的兜底结构化结果（保持接口稳定）。"""
        return ParsedData(
            characters=[],
            scene={},
            objects=[],
            actions=[],
            emotions=[],
            style={"fallback": True, "source_text_length": len(text)},
        )

    @staticmethod
    def dump(data: ParsedData) -> str:
        """调试用：把解析结果序列化为 JSON 字符串。"""
        return json.dumps(data.model_dump(), ensure_ascii=False)
