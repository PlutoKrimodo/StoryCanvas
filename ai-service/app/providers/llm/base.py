"""LLM Provider 抽象。

约定 `generate` 返回 **纯文本**（模型输出的原始字符串）；
JSON 结构的解析由上层 `TextAnalyzerService` 负责，Provider 不感知业务语义。
"""

from __future__ import annotations

from abc import abstractmethod
from typing import Dict, List

from ..base import BaseProvider

Message = Dict[str, str]


class LLMProvider(BaseProvider):
    """大语言模型 Provider 基类。"""

    @abstractmethod
    async def generate(
        self,
        messages: List[Message],
        *,
        temperature: float = 0.7,
        max_tokens: int = 1000,
        json_mode: bool = False,
    ) -> str:
        """根据对话消息生成文本。

        Args:
            messages: OpenAI 风格的 `[{"role": ..., "content": ...}]`
            temperature: 采样温度，文本解析建议取较低值（如 0.3）
            max_tokens: 最大生成 token 数
            json_mode: 是否要求模型返回 JSON 对象
        """
