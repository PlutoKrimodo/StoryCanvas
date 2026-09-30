"""文本分析服务（Phase 4）。

职责：把用户故事文本交给 LLM，得到结构化结果（角色 / 场景 / 物品 / 动作 / 风格）。
Robust 的 JSON 抽取放在这里，使 Provider 与业务解耦。
"""

from __future__ import annotations

import json
import re
from typing import Any, Dict

from ..models.schemas import AnalysisResult, ParsedData
from ..prompts.text_analysis import TextAnalysisPrompt
from ..providers.llm.base import LLMProvider
from ..utils.exceptions import AIServiceException
from .base import BaseService

# 匹配 ```json ... ``` / ``` ... ``` 代码块
_FENCE_RE = re.compile(r"```(?:json)?\s*(.*?)\s*```", re.DOTALL)


class TextAnalyzerService(BaseService):
    """文本分析服务。"""

    def __init__(self, llm_provider: LLMProvider) -> None:
        super().__init__()
        self.llm_provider = llm_provider
        self.prompt_builder = TextAnalysisPrompt()

    async def analyze(self, text: str) -> AnalysisResult:
        """分析文本并返回结构化结果。"""
        prompt = self.prompt_builder.build_prompt(text)
        try:
            raw = await self.llm_provider.generate(
                prompt, temperature=0.3, max_tokens=1200, json_mode=True
            )
        except AIServiceException:
            raise
        except Exception as exc:  # Provider 抛出的非业务异常统一包装
            self.logger.error("文本分析失败: %s", exc)
            raise AIServiceException(f"文本分析失败: {exc}", 502) from exc

        parsed = self._parse_response(raw)
        return AnalysisResult(parsed_data=parsed, confidence=0.9, raw_response=raw)

    # ------------------------------------------------------------------
    # 内部：健壮的 JSON 提取
    # ------------------------------------------------------------------
    def _parse_response(self, raw: str) -> ParsedData:
        """从模型输出中抽取 JSON 并映射为 ParsedData。

        模型可能包裹 markdown 代码块或在 JSON 前后附带说明文字，
        因此按「代码块 → 首尾大括号 → 兜底」三级策略解析。
        """
        candidate = self._extract_json_text(raw)
        if candidate:
            try:
                data = json.loads(candidate)
                if isinstance(data, dict):
                    return self._coerce(data)
            except json.JSONDecodeError as exc:
                self.logger.warning("JSON 解析失败，使用兜底结果: %s", exc)
        return TextAnalysisPrompt.fallback(raw)

    @staticmethod
    def _extract_json_text(raw: str) -> str:
        text = (raw or "").strip()
        if not text:
            return ""

        fence = _FENCE_RE.search(text)
        if fence:
            text = fence.group(1).strip()

        start = text.find("{")
        end = text.rfind("}")
        if start != -1 and end != -1 and end > start:
            return text[start : end + 1]
        return ""

    @staticmethod
    def _coerce(data: Dict[str, Any]) -> ParsedData:
        """宽松映射：容忍模型返回非严格结构。"""
        scene = data.get("scene")
        if not isinstance(scene, dict):
            scene = {}

        characters = data.get("characters")
        if not isinstance(characters, list):
            characters = []

        def as_list(value: Any) -> list:
            if isinstance(value, list):
                return [str(item) for item in value if str(item).strip()]
            if isinstance(value, str) and value.strip():
                return [value.strip()]
            return []

        style = data.get("style")
        if not isinstance(style, dict):
            style = {}

        return ParsedData(
            characters=characters,
            scene=scene,  # pydantic 会把缺失字段填默认值
            objects=as_list(data.get("objects")),
            actions=as_list(data.get("actions")),
            emotions=as_list(data.get("emotions")),
            style=style,
        )
