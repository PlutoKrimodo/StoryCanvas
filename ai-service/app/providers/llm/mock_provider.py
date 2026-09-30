"""Mock LLM Provider（开发 / 测试 / 故障兜底）。

不依赖任何外部服务，用简单的关键字词典从输入文本里抽取
「角色 / 场景 / 物品 / 动作 / 情感」，输出与真实模型一致的 JSON 结构，
保证 `LLM_PROVIDER=mock` 时可完全离线跑通整条链路。
"""

from __future__ import annotations

import json
import re
from typing import Dict, List

from .base import LLMProvider, Message

# 简易词表：命中即作为结构化字段候选
_CHARACTER_WORDS = [
    "小女孩", "小男孩", "小女孩儿", "妈妈", "爸爸", "奶奶", "爷爷", "老师",
    "兔子", "小鸟", "小猫", "小狗", "狐狸", "小熊", "熊猫", "小鹿", "蝴蝶",
    "王子", "公主", "爷爷", "奶奶", "朋友",
]
_OBJECT_WORDS = [
    "森林", "树", "大树", "花", "花朵", "篮子", "苹果", "星星", "月亮", "太阳",
    "雨伞", "风筝", "气球", "书本", "绘本", "小船", "房子", "蘑菇", "河流", "石头",
]
_ACTION_WORDS = [
    "发现", "跑", "走", "飞", "唱歌", "跳舞", "微笑", "哭", "寻找", "帮助",
    "坐在", "躺在", "捡起", "拥抱", "挥手", "看着", "等待", "回家",
]
_EMOTION_WORDS = ["开心", "快乐", "温暖", "勇敢", "害怕", "难过", "惊喜", "好奇", "安静", "幸福"]
_LOCATION_WORDS = ["森林", "海边", "城市", "家", "花园", "山谷", "草地", "河边", "学校", "天空"]
_TIME_WORDS = ["清晨", "早上", "中午", "下午", "傍晚", "夜晚", "晚上", "春天", "夏天", "秋天", "冬天"]
_WEATHER_WORDS = ["雨后", "晴天", "下雨", "下雪", "刮风", "阳光", "多云", "彩虹"]
_MOOD_WORDS = ["温馨", "梦幻", "安静", "欢快", "神秘", "治愈", "热闹"]


def _find_words(text: str, lexicon: List[str]) -> List[str]:
    """按出现顺序返回命中的词（去重）。"""
    hits: List[str] = []
    for word in lexicon:
        if word in text and word not in hits:
            hits.append(word)
    return hits


class MockLLMProvider(LLMProvider):
    """基于词典的确定性 Mock 实现。"""

    @property
    def name(self) -> str:
        return "mock-llm"

    async def generate(
        self,
        messages: List[Message],
        *,
        temperature: float = 0.7,
        max_tokens: int = 1000,
        json_mode: bool = False,
    ) -> str:
        text = self._extract_text(messages)
        if json_mode:
            return json.dumps(self.analyze_text(text), ensure_ascii=False)
        return f"mock-analysis: {text[:60]}"

    async def health_check(self) -> bool:
        return True

    # ------------------------------------------------------------------
    # 内部工具
    # ------------------------------------------------------------------
    @staticmethod
    def _extract_text(messages: List[Message]) -> str:
        """取最后一条 user 消息作为待分析文本。"""
        for message in reversed(messages):
            if message.get("role") == "user":
                return str(message.get("content", ""))
        return ""

    def analyze_text(self, text: str) -> Dict[str, object]:
        """把自然语言线索抽取为与 ParsedData 对齐的字典。"""
        characters = _find_words(text, _CHARACTER_WORDS)
        objects = _find_words(text, _OBJECT_WORDS)
        actions = _find_words(text, _ACTION_WORDS)
        emotions = _find_words(text, _EMOTION_WORDS)
        locations = _find_words(text, _LOCATION_WORDS)
        times = _find_words(text, _TIME_WORDS)
        weathers = _find_words(text, _WEATHER_WORDS)
        moods = _find_words(text, _MOOD_WORDS)

        # 未识别到角色时，从句子中兜底取一个"主角"，保证结构化结果非空
        if not characters:
            match = re.search(r"([\u4e00-\u9fa5]{2,4})(?:在|把|和|与|对)", text)
            characters = [match.group(1)] if match else ["小朋友"]

        return {
            "characters": [
                {
                    "name": name,
                    "description": f"{name}是故事里的重要角色",
                    "appearance": "",
                    "position": "",
                }
                for name in characters[:3]
            ],
            "scene": {
                "location": locations[0] if locations else "",
                "time": times[0] if times else "",
                "weather": weathers[0] if weathers else "",
                "mood": moods[0] if moods else "",
            },
            "objects": objects,
            "actions": actions,
            "emotions": emotions,
            "style": {"source": "mock"},
        }
