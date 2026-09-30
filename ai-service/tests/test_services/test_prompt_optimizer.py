"""Prompt 优化服务单元测试。"""

from app.models.schemas import ArtStyle, ParsedCharacter, ParsedData, ParsedScene
from app.services.prompt_optimizer import PromptOptimizerService


def _sample() -> ParsedData:
    return ParsedData(
        characters=[ParsedCharacter(name="小女孩", appearance="红色斗篷")],
        scene=ParsedScene(location="森林", mood="温馨"),
        objects=["小鸟"],
        actions=["发现"],
        emotions=["好奇"],
    )


def test_optimize_includes_style_keywords_and_entities():
    service = PromptOptimizerService()
    prompt = service.optimize(_sample(), ArtStyle.WATERCOLOR, None)

    assert "watercolor painting" in prompt
    assert "小女孩" in prompt
    assert "红色斗篷" in prompt
    assert "森林" in prompt
    assert "小鸟" in prompt


def test_negative_prompt_from_style_template():
    service = PromptOptimizerService()
    negative = service.negative(ArtStyle.CARTOON)
    assert "scary" in negative


def test_unknown_style_falls_back_to_cartoon():
    service = PromptOptimizerService()
    template = service.style_template("not-a-style")
    assert template["key"] == "cartoon"


def test_additional_keywords_appended():
    service = PromptOptimizerService()
    prompt = service.optimize(
        _sample(), ArtStyle.CARTOON, {"extra_keywords": "golden hour lighting"}
    )
    assert "golden hour lighting" in prompt
