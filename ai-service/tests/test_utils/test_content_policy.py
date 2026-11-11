"""儿童绘本内容策略：文本预检、厂商拒绝归一与输出复核状态。"""

import httpx
import pytest

from app.services import content_policy
from app.utils.exceptions import CONTENT_REJECTED, ContentPolicyException


def test_safe_children_story_passes():
    decision = content_policy.evaluate_text(
        "小女孩在雨后的森林里发现一只受伤的小鸟，把它带回家照顾。"
    )
    assert decision.allowed is True


@pytest.mark.parametrize(
    "text",
    [
        "这是一段色情内容",
        "画面出现血腥和肢解",
        "请画米老鼠和皮卡丘",
        "故事包含自杀桥段",
    ],
)
def test_high_risk_text_is_rejected(text):
    with pytest.raises(ContentPolicyException) as exc:
        content_policy.assert_allowed(text)
    assert exc.value.code == 422
    assert exc.value.error_code == CONTENT_REJECTED
    assert "内容不适合儿童绘本" in exc.value.message


def test_negative_prompt_words_are_not_scanned_as_story_text():
    # 负向 Prompt 中本来就会出现这些排除词，review_output 只检查正面 Prompt。
    status, reason = content_policy.review_output(
        provider_name="mock-image",
        prompt="a gentle children's book illustration of a girl and a bird",
        negative_prompt="violent, gore, scary, horror, watermark",
        finish_reason="success",
        provider_metadata=None,
    )
    assert status == content_policy.CONTENT_REVIEW_APPROVED
    assert "输入策略通过" in reason


def test_provider_http_error_is_normalized():
    request = httpx.Request("POST", "https://api.example.com/chat/completions")
    response = httpx.Response(
        400,
        request=request,
        json={"error": {"code": "content_filter", "message": "sensitive content detected"}},
    )
    error = httpx.HTTPStatusError("bad request", request=request, response=response)

    rejection = content_policy.provider_rejection_from_exception(error, "DeepSeek")

    assert rejection is not None
    assert rejection.error_code == CONTENT_REJECTED
    assert rejection.code == 422


def test_provider_metadata_can_require_manual_review():
    status, reason = content_policy.review_output(
        provider_name="doubao",
        prompt="a watercolor illustration of a child planting a tree",
        negative_prompt="watermark",
        finish_reason="success",
        provider_metadata={"content_filter_results": {"status": "review_required"}},
    )
    assert status == content_policy.CONTENT_REVIEW_PENDING
    assert "人工复核" in reason


def test_provider_metadata_blocked_raises_content_rejection():
    with pytest.raises(ContentPolicyException) as exc:
        content_policy.review_output(
            provider_name="wanx",
            prompt="a watercolor illustration of a child planting a tree",
            negative_prompt="watermark",
            finish_reason="success",
            provider_metadata={"content_filter_results": {"status": "blocked"}},
        )
    assert exc.value.error_code == CONTENT_REJECTED
