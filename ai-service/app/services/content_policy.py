"""StoryCanvas 儿童绘本内容策略。

这是面向课程项目的轻量规则层，目标是把明显不适合儿童绘本的文本、
厂商内容拒绝和输出结果状态统一为 ``CONTENT_REJECTED`` / ``approved`` /
``pending_review`` 三类可追踪信号。

能力边界：
- 能对用户输入、分镜文本、正面 Prompt 做确定性规则预检；
- 能归一厂商返回的敏感内容 / 版权 / 策略拒绝；
- 能记录生成图的内容复核状态；
- 不做像素级图像分类，不替代人工审核或生产级审核服务。
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any, Iterable, Optional

import httpx

from ..utils.exceptions import CONTENT_REJECTED, ContentPolicyException

CONTENT_REVIEW_APPROVED = "approved"
CONTENT_REVIEW_PENDING = "pending_review"
CONTENT_REVIEW_REJECTED = "rejected"


@dataclass(frozen=True)
class ContentDecision:
    """一次文本策略判断的结果。"""

    allowed: bool
    category: str = ""
    label: str = ""
    message: str = ""
    matched: str = ""


# 规则只用于拦截高风险、明确不适合儿童绘本的表述，避免把“受伤的小鸟”
# “勇敢”“夜晚”“龙”等正常童话元素误判为违规。
_RULES: tuple[tuple[str, str, tuple[str, ...]], ...] = (
    (
        "sexual",
        "性暗示或裸露内容",
        (
            r"色情", r"裸体", r"裸露", r"性行为", r"性交", r"成人内容",
            r"\bporn(?:ography)?\b", r"\bnud(?:e|ity)\b", r"\bsexual\b", r"\berotic\b",
        ),
    ),
    (
        "violence_gore",
        "血腥或重度暴力画面",
        (
            r"血腥", r"肢解", r"虐杀", r"砍杀", r"斩首", r"酷刑", r"枪战",
            r"\bgraphic violence\b", r"\bgore\b", r"\bmurder\b", r"\bmassacre\b",
            r"\bdismember\b", r"\bbehead\b", r"\btorture\b",
        ),
    ),
    (
        "horror",
        "惊悚恐怖画面",
        (
            r"恐怖袭击", r"恐怖画面", r"惊悚", r"灵异", r"血腥恐怖",
            r"\bhorror\b", r"\bterrifying\b", r"\bnightmare fuel\b",
        ),
    ),
    (
        "hate",
        "仇恨或歧视内容",
        (
            r"种族歧视", r"仇恨言论", r"纳粹", r"\bhate speech\b",
            r"\bracial slur\b", r"\bgenocide\b",
        ),
    ),
    (
        "self_harm",
        "自伤或自杀内容",
        (
            r"自杀", r"自残", r"割腕", r"上吊", r"\bsuicide\b",
            r"\bself[- ]harm\b",
        ),
    ),
    (
        "real_person",
        "真实人物肖像",
        (
            r"真实人物", r"政治人物", r"当红明星", r"名人肖像",
            r"\breal person\b", r"\bcelebrity\b", r"\bpolitician\b",
        ),
    ),
    (
        "copyright",
        "明显受版权保护的角色或IP",
        (
            r"米老鼠", r"迪士尼", r"皮卡丘", r"宝可梦", r"哈利波特", r"漫威",
            r"\bmickey mouse\b", r"\bdisney\b", r"\bpikachu\b",
            r"\bpok(?:e|é)mon\b", r"\bharry potter\b", r"\bmarvel\b",
        ),
    ),
)

_PROVIDER_REJECTION_MARKERS = (
    "sensitivecontentdetected",
    "policyviolation",
    "content_filter",
    "content filter",
    "content_policy",
    "content policy",
    "sensitive",
    "敏感",
    "内容安全",
    "版权",
    "copyright",
    "moderation",
    "unsafe",
    "blocked",
    "review_required",
)


def _message(category: str, label: str) -> str:
    return (
        f"内容不适合儿童绘本：检测到{label}。"
        "请修改故事、角色设定或分镜描述后再试；这类失败不会自动重试。"
    )


def evaluate_text(*parts: Optional[str]) -> ContentDecision:
    """对一段或多段正面文本执行规则预检。

    只应传入用户故事、分镜描述、标题或正面 Prompt。负向 Prompt 中本来就会
    出现 scary / violent 等排除词，不应送入本函数。
    """

    text = " ".join(str(part) for part in parts if part)
    if not text.strip():
        return ContentDecision(True)

    for category, label, patterns in _RULES:
        for pattern in patterns:
            matched = re.search(pattern, text, flags=re.IGNORECASE)
            if matched:
                return ContentDecision(
                    allowed=False,
                    category=category,
                    label=label,
                    message=_message(category, label),
                    matched=matched.group(0),
                )
    return ContentDecision(True)


def assert_allowed(*parts: Optional[str]) -> None:
    """文本不合规则时抛出统一的 CONTENT_REJECTED 异常。"""

    decision = evaluate_text(*parts)
    if decision.allowed:
        return
    raise ContentPolicyException(decision.message, category=decision.category)


def provider_rejection(
    provider: str,
    status: Optional[int],
    code: str,
    message: str,
) -> Optional[ContentPolicyException]:
    """把厂商敏感内容 / 版权 / 策略拒绝归一为内容拒绝异常。

    只检查结构化的错误码和消息，不把完整响应体写入日志或用户提示。
    """

    haystack = f"{code} {message}".lower()
    if not any(marker in haystack for marker in _PROVIDER_REJECTION_MARKERS):
        return None
    if status is not None and status not in (400, 403, 422, 451, 500, 502):
        return None
    return ContentPolicyException(
        "图像涉及版权或内容安全限制，请使用原创角色与场景后重试",
        category=f"provider:{provider}",
    )


def provider_rejection_from_exception(
    exc: Exception,
    provider: str,
) -> Optional[ContentPolicyException]:
    """从 httpx HTTP 异常中提取结构化厂商错误码并判断是否内容拒绝。"""

    status: Optional[int] = None
    code = ""
    message = ""
    if isinstance(exc, httpx.HTTPStatusError):
        status = exc.response.status_code
        try:
            body = exc.response.json()
            error = body.get("error") if isinstance(body, dict) else None
            if isinstance(error, dict):
                code = str(error.get("code") or error.get("type") or "")
                message = str(error.get("message") or "")
            elif isinstance(error, str):
                message = error
        except Exception:
            # 不读取 response.text：厂商可能回显请求体或凭据。
            code = ""
            message = ""
    else:
        message = str(exc)
    return provider_rejection(provider, status, code, message)


def _content_filter_info(metadata: Optional[dict[str, Any]]) -> dict[str, Any]:
    if not isinstance(metadata, dict):
        return {}
    info = metadata.get("content_filter_results") or metadata.get("content_filter")
    return info if isinstance(info, dict) else {}


def review_output(
    *,
    provider_name: str,
    prompt: str,
    negative_prompt: str,
    finish_reason: str,
    provider_metadata: Optional[dict[str, Any]],
) -> tuple[str, str]:
    """记录生成图输出复核状态。

    当前课程项目没有本地图像分类模型，因此这里不宣称“识别了图片像素”。
    它会检查：正面 Prompt 是否仍合规、厂商是否返回内容过滤元数据、任务
    finish_reason 是否表示被过滤；全部通过时记录为 approved，厂商要求人工
    复核时记录为 pending_review，明确拒绝时直接抛出 CONTENT_REJECTED。
    """

    # 负向 Prompt 只用于排除元素，不参与内容规则匹配。
    assert_allowed(prompt)

    finish = (finish_reason or "").lower()
    if any(word in finish for word in ("content_filter", "blocked", "rejected", "sensitive")):
        raise ContentPolicyException(
            "图像生成被内容策略拦截，请使用原创角色与场景后重试",
            category=f"output:{provider_name}",
        )

    info = _content_filter_info(provider_metadata)
    status = str(info.get("status") or info.get("decision") or "").lower()
    blocked = info.get("blocked") is True
    if blocked or status in ("blocked", "rejected", "reject", "violation"):
        raise ContentPolicyException(
            "图像涉及版权或内容安全限制，请使用原创角色与场景后重试",
            category=f"output:{provider_name}",
        )
    if status in ("review_required", "review", "pending", "flagged"):
        return (
            CONTENT_REVIEW_PENDING,
            "厂商返回需人工复核；该图片暂不允许加入绘本，请重新生成或更换描述。",
        )

    return (
        CONTENT_REVIEW_APPROVED,
        "输入策略通过；厂商未返回内容风险标记。规则层已记录，非像素级图像分类。",
    )
