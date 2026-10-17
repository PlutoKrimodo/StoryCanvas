"""分镜服务：JSON 映射、数量兜底与一致性设定。"""

import json

import pytest

from app.models.schemas import ArtStyle
from app.services.storyboard_service import StoryboardService


class FakeProvider:
    def __init__(self, payload: str) -> None:
        self.payload = payload
        self.calls = 0

    async def generate(self, *args, **kwargs) -> str:
        self.calls += 1
        return self.payload


@pytest.mark.asyncio
async def test_storyboard_parses_llm_json():
    payload = {
        "title": "雨后的小鸟",
        "parsed_data": {
            "characters": [{"name": "小女孩", "appearance": "红色雨衣"}],
            "scene": {"location": "森林"},
            "objects": ["小鸟"],
            "actions": ["发现"],
            "emotions": ["心疼"],
            "style": {},
        },
        "consistency": {
            "characters": [{"name": "小女孩", "appearance": "红色雨衣、黄色雨靴"}],
            "palette": "暖黄与苔绿",
            "notes": "水彩，柔和边缘",
        },
        "panels": [
            {"index": 0, "text": "雨后的森林，小女孩蹲下发现小鸟"},
            {"index": 1, "text": "小女孩把小鸟捧在手心"},
        ],
        "negative_prompt": "角色变形",
    }
    service = StoryboardService(FakeProvider(json.dumps(payload, ensure_ascii=False)))

    result = await service.generate("故事原文", 2, ArtStyle.WATERCOLOR)

    assert result.title == "雨后的小鸟"
    assert len(result.panels) == 2
    assert result.panels[1].index == 1
    assert result.consistency.characters[0].appearance == "红色雨衣、黄色雨靴"
    assert "same character design" in result.consistency_prompt
    assert result.negative_prompt == "角色变形"


@pytest.mark.asyncio
async def test_invalid_json_falls_back_to_deterministic_split():
    service = StoryboardService(FakeProvider("这不是 JSON"))
    text = "第一句。第二句。第三句。"

    result = await service.generate(text, 3, ArtStyle.CARTOON)

    assert len(result.panels) == 3
    assert [panel.index for panel in result.panels] == [0, 1, 2]
    assert all(panel.text for panel in result.panels)


@pytest.mark.asyncio
async def test_panel_count_mismatch_is_padded_or_truncated():
    payload = {
        "panels": [{"index": 0, "text": "只有一格"}],
        "parsed_data": {"characters": [{"name": "小熊"}], "scene": {}},
    }
    service = StoryboardService(FakeProvider(json.dumps(payload, ensure_ascii=False)))

    result = await service.generate("小熊去河边。小熊回家。", 3, ArtStyle.CARTOON)

    assert len(result.panels) == 3
    assert result.panels[0].text == "只有一格"
    assert all(panel.text for panel in result.panels)
