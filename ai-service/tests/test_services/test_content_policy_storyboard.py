"""内容策略拒绝不能被 StoryboardService 的确定性兜底绕过。"""

import pytest

from app.models.schemas import ArtStyle
from app.services.storyboard_service import StoryboardService
from app.utils.exceptions import CONTENT_REJECTED, ContentPolicyException


class RejectingProvider:
    def __init__(self) -> None:
        self.calls = 0

    async def generate(self, *args, **kwargs) -> str:
        self.calls += 1
        raise ContentPolicyException(
            "内容不适合儿童绘本：检测到血腥或重度暴力画面。",
            category="violence_gore",
        )


@pytest.mark.asyncio
async def test_content_policy_rejection_does_not_fall_back():
    provider = RejectingProvider()
    service = StoryboardService(provider)  # type: ignore[arg-type]

    with pytest.raises(ContentPolicyException) as exc:
        await service.generate("一只小猫在花园里晒太阳", 2, ArtStyle.CARTOON)

    assert provider.calls == 1
    assert exc.value.error_code == CONTENT_REJECTED
    assert exc.value.code == 422
