"""Mock LLM Provider 单元测试。"""

import json

import pytest

from app.models.schemas import ParsedData
from app.providers.llm.mock_provider import MockLLMProvider


@pytest.mark.asyncio
async def test_generate_json_mode_returns_parsed_structure():
    provider = MockLLMProvider()
    messages = [
        {"role": "system", "content": "return json"},
        {"role": "user", "content": "小女孩在雨后的森林里发现了一只受伤的小鸟。"},
    ]

    raw = await provider.generate(messages, json_mode=True)
    data = json.loads(raw)

    # 输出必须能映射为 ParsedData（与真实模型结构一致）
    parsed = ParsedData(**data)
    assert parsed.characters, "应至少识别出一个角色"
    assert "森林" in parsed.scene.location or parsed.scene.location


@pytest.mark.asyncio
async def test_generate_plain_text_mode():
    provider = MockLLMProvider()
    raw = await provider.generate([{"role": "user", "content": "你好"}], json_mode=False)
    assert "你好" in raw


@pytest.mark.asyncio
async def test_health_check_true():
    assert await MockLLMProvider().health_check() is True


def test_analyze_text_fallback_character():
    """未命中词表时应兜底出一个主角，保证结构化结果非空。"""
    provider = MockLLMProvider()
    result = provider.analyze_text("阿宝在草地上玩。")
    assert len(result["characters"]) >= 1
