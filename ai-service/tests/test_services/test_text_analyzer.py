"""文本分析服务单元测试。"""

import pytest

from app.models.schemas import ParsedData
from app.providers.llm.mock_provider import MockLLMProvider
from app.services.text_analyzer import TextAnalyzerService
from app.utils.exceptions import AIServiceException


@pytest.mark.asyncio
async def test_analyze_returns_structured_data():
    service = TextAnalyzerService(MockLLMProvider())
    result = await service.analyze("小女孩在雨后的森林里发现了一只受伤的小鸟。")

    assert isinstance(result.parsed_data, ParsedData)
    assert result.confidence > 0
    assert result.parsed_data.characters


def test_parse_response_strips_markdown_fence():
    service = TextAnalyzerService(MockLLMProvider())
    raw = '```json\n{"characters": [{"name": "小明"}], "scene": {}, "objects": ["伞"]}\n```'
    parsed = service._parse_response(raw)
    assert parsed.characters[0].name == "小明"
    assert parsed.objects == ["伞"]


def test_parse_response_falls_back_on_invalid_json():
    service = TextAnalyzerService(MockLLMProvider())
    parsed = service._parse_response("not a json at all")
    assert isinstance(parsed, ParsedData)
    assert parsed.style.get("fallback") is True


def test_coerce_tolerates_non_strict_types():
    service = TextAnalyzerService(MockLLMProvider())
    parsed = service._parse_response(
        '{"characters": "单个角色", "scene": "不是对象", "objects": "花", "actions": [], "emotions": []}'
    )
    # characters 非列表时按空处理，objects 字符串被包成单元素列表
    assert parsed.characters == []
    assert parsed.objects == ["花"]


@pytest.mark.asyncio
async def test_analyze_wraps_provider_error():
    class BoomProvider(MockLLMProvider):
        async def generate(self, *args, **kwargs):
            raise RuntimeError("boom")

    service = TextAnalyzerService(BoomProvider())
    with pytest.raises(AIServiceException):
        await service.analyze("任意文本")
