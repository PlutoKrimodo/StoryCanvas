"""分镜规划服务（Storyboard Chat）。

一次 LLM 调用产出「结构化解析 + N 格分镜 + 一致性设定」；
LLM 输出不合法或网络异常时退化为确定性切分，保证对话流程不中断。
"""

from __future__ import annotations

import json
import re
from typing import Any, Dict, List, Optional

from ..models.schemas import (
    ArtStyle,
    ParsedData,
    StoryboardCharacter,
    StoryboardConsistency,
    StoryboardPanel,
    StoryboardResponse,
)
from ..prompts.storyboard import StoryboardPrompt
from ..providers.llm.base import LLMProvider
from ..utils.exceptions import AIServiceException
from .base import BaseService
from .prompt_optimizer import PromptOptimizerService
from .text_analyzer import TextAnalyzerService

_FENCE_RE = re.compile(r"```(?:json)?\s*(.*?)\s*```", re.DOTALL)
_FAST_FAIL_CODES = (401, 402, 403, 429)

# 单格文本清洗：格号前缀、句中格号、组级说明、多格触发词
_PANEL_PREFIX_RE = re.compile(
    r"^\s*(?:第\s*[0-9一二三四五六七八九十]+\s*[格幅张画面]*|panel\s*\d+|画面\s*\d+)"
    r"\s*[：:、.．]\s*",
    re.IGNORECASE,
)
_PANEL_MARK_RE = re.compile(
    r"(?:第\s*[0-9一二三四五六七八九十]+\s*[格幅张画面]|panel\s*\d+)",
    re.IGNORECASE,
)
_GROUP_LEVEL_RE = re.compile(
    r"(整组统一|所有画面|每一格|全部画面|保持同一画风|组图|连环画)", re.IGNORECASE
)
_MULTI_PANEL_WORD_RE = re.compile(
    r"(storyboard|comic|multi-?panel|split[\s-]?screen|collage|grid)", re.IGNORECASE
)
_SENTENCE_SPLIT_RE = re.compile(r"[。；;！？!?\n]+")
_WHITESPACE_RE = re.compile(r"\s+")


def sanitize_panel_text(text: str) -> str:
    """清洗单格描述，去掉格号、组级说明与多格触发词。

    保守策略：清洗后为空或异常变短时回退原文，避免误删正常画面描述。
    返回长度上限 500，与 ``StoryboardPanel.text`` 保持一致。
    """
    original = (text or "").strip()
    if not original:
        return original

    cleaned = _PANEL_PREFIX_RE.sub("", original)
    cleaned = _PANEL_MARK_RE.sub("", cleaned)
    cleaned = _MULTI_PANEL_WORD_RE.sub("", cleaned)

    # 组级说明（“整组统一/所有画面/每一格…”）不属于单格画面，按句过滤
    sentences = [item.strip() for item in _SENTENCE_SPLIT_RE.split(cleaned) if item.strip()]
    kept = [item for item in sentences if not _GROUP_LEVEL_RE.search(item)]
    if kept:
        cleaned = "。".join(kept)
    else:
        cleaned = "。"

    cleaned = _WHITESPACE_RE.sub(" ", cleaned).strip(" ，,、。；;")
    cleaned = re.sub(r"[。；;]{2,}", "。", cleaned)

    if not cleaned:
        return original
    # 清洗后异常变短（超过 80% 内容被移除）时回退，避免误删正常画面描述
    if len(original) >= 20 and len(cleaned) < len(original) * 0.2:
        return original
    return cleaned[:500]


class StoryboardService(BaseService):
    """把故事拆成 N 格，并给出整组共享的一致性设定。"""

    def __init__(self, llm_provider: LLMProvider) -> None:
        super().__init__()
        self.llm_provider = llm_provider
        self.prompt_builder = StoryboardPrompt()
        self.optimizer = PromptOptimizerService()

    async def generate(
        self,
        text: str,
        image_count: int,
        style: ArtStyle | str,
    ) -> StoryboardResponse:
        messages = self.prompt_builder.build_prompt(text, image_count, style)
        data: Dict[str, Any] = {}

        try:
            # 张数越多，JSON 输出越长；按格数放大 token 预算，避免 12 格时被截断
            max_tokens = min(4096, max(1600, image_count * 220))
            raw = await self.llm_provider.generate(
                messages, temperature=0.4, max_tokens=max_tokens, json_mode=True
            )
            parsed = self._extract_json(raw)
            if isinstance(parsed, dict):
                data = parsed
        except AIServiceException as exc:
            if exc.code in _FAST_FAIL_CODES:
                raise
            self.logger.warning("分镜 LLM 调用失败，使用确定性兜底: %s", exc.message)
        except Exception as exc:  # Provider 之外的非预期异常
            self.logger.warning("分镜 LLM 调用异常，使用确定性兜底: %s", exc)

        panels = self._normalize_panels(text, image_count, data.get("panels"))
        parsed_data = await self._resolve_parsed_data(text, data.get("parsed_data"))
        consistency = self._build_consistency(data.get("consistency"), parsed_data)
        # 风格由图像阶段的 style 模板统一注入，这里只固定角色与配色，避免两处冲突
        consistency_prompt = self._build_consistency_prompt(consistency)
        negative = str(data.get("negative_prompt") or "").strip()
        if not negative:
            negative = self.optimizer.negative(style)

        title = str(data.get("title") or "").strip()
        if not title:
            title = (text.strip().splitlines()[0] if text.strip() else "新的绘本故事")[:24]

        return StoryboardResponse(
            title=title[:120],
            parsed_data=parsed_data,
            consistency=consistency,
            consistency_prompt=consistency_prompt,
            panels=panels,
            negative_prompt=negative[:500],
        )

    # ------------------------------------------------------------------
    # JSON 与字段容错
    # ------------------------------------------------------------------
    @staticmethod
    def _extract_json(raw: str) -> Optional[Dict[str, Any]]:
        text = (raw or "").strip()
        if not text:
            return None
        fence = _FENCE_RE.search(text)
        if fence:
            text = fence.group(1).strip()
        start = text.find("{")
        end = text.rfind("}")
        if start == -1 or end <= start:
            return None
        try:
            value = json.loads(text[start : end + 1])
        except json.JSONDecodeError:
            return None
        return value if isinstance(value, dict) else None

    def _normalize_panels(
        self, text: str, image_count: int, raw_panels: Any
    ) -> List[StoryboardPanel]:
        texts: List[str] = []
        if isinstance(raw_panels, list):
            for item in raw_panels:
                if isinstance(item, dict):
                    value = str(item.get("text") or "").strip()
                else:
                    value = str(item or "").strip()
                if value:
                    texts.append(value[:500])

        fallback = self.prompt_builder.split_fallback(text, image_count)
        if not texts:
            texts = fallback
        while len(texts) < image_count:
            texts.append(fallback[len(texts) % len(fallback)])
        texts = texts[:image_count]
        # LLM 与确定性兜底都可能带“第 X 格”或组级说明，统一清洗
        texts = [sanitize_panel_text(value) for value in texts]

        return [
            StoryboardPanel(index=index, text=value)
            for index, value in enumerate(texts)
        ]

    async def _resolve_parsed_data(self, text: str, raw: Any) -> ParsedData:
        if isinstance(raw, dict):
            try:
                parsed = TextAnalyzerService._coerce(raw)
                if parsed.characters or parsed.objects or parsed.scene.location:
                    return parsed
            except Exception:  # 模型返回的字段类型可能不稳定
                self.logger.warning("分镜 parsed_data 无法映射，尝试重新解析")

        try:
            analysis = await TextAnalyzerService(self.llm_provider).analyze(text)
            return analysis.parsed_data
        except Exception as exc:
            # 兜底：至少保证后续 Prompt 优化有合法的空结构，不阻断出图
            self.logger.warning("兜底文本解析失败，使用空结构: %s", exc)
            return ParsedData()

    @staticmethod
    def _build_consistency(raw: Any, parsed: ParsedData) -> StoryboardConsistency:
        characters: List[StoryboardCharacter] = []
        palette = ""
        notes = ""

        if isinstance(raw, dict):
            palette = str(raw.get("palette") or "")[:120]
            notes = str(raw.get("notes") or "")[:300]
            for item in raw.get("characters") or []:
                if isinstance(item, dict) and str(item.get("name") or "").strip():
                    characters.append(
                        StoryboardCharacter(
                            name=str(item["name"])[:50],
                            appearance=str(item.get("appearance") or "")[:300],
                        )
                    )

        if not characters:
            for character in parsed.characters[:5]:
                characters.append(
                    StoryboardCharacter(
                        name=character.name[:50],
                        appearance=(character.appearance or character.description or "")[:300],
                    )
                )

        return StoryboardConsistency(
            characters=characters,
            palette=palette,
            notes=notes,
        )

    @staticmethod
    def _build_consistency_prompt(consistency: StoryboardConsistency) -> str:
        parts = [
            "Keep the same character design, clothing and art style in this single illustration.",
            "One continuous scene, full-bleed, no panels, no split screen, no collage, no grid.",
        ]
        for character in consistency.characters:
            if character.appearance:
                parts.append(f"{character.name}: {character.appearance}")
            else:
                parts.append(character.name)
        if consistency.palette:
            parts.append(f"Color palette: {consistency.palette}")
        if consistency.notes:
            parts.append(consistency.notes)
        return " ".join(parts)[:1000]
