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


def test_optimize_panel_is_single_scene():
    service = PromptOptimizerService()
    parsed = ParsedData(
        characters=[ParsedCharacter(name="小女孩", appearance="草帽、白裙")],
        scene=ParsedScene(location="森林", mood="温馨"),
        objects=["小鸟"],
        actions=["发现"],
        emotions=["好奇"],
    )

    prompt = service.optimize_panel(
        "小女孩坐在旧木船上，手捧白色野花",
        parsed,
        ArtStyle.OIL_PAINTING,
    )

    # 单图约束与当前格场景
    assert "single continuous illustration" in prompt
    assert "no split screen" in prompt
    assert "小女孩坐在旧木船上" in prompt
    # 角色外观仍保留，用于跨图一致性
    assert "草帽、白裙" in prompt
    # 整段故事的场景 / 物品 / 动作不得混入单格
    assert "森林" not in prompt
    assert "小鸟" not in prompt
    assert "发现" not in prompt


def test_anti_panel_negative_contains_split_screen():
    service = PromptOptimizerService()
    negative = service.anti_panel_negative()

    assert "split screen" in negative
    assert "multiple panels" in negative
    assert "comic strip" in negative
