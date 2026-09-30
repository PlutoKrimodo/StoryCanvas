"""AI Provider 基类（Provider 抽象，对照 docs/16 §4.2 决策 7）。"""

from __future__ import annotations

from abc import ABC, abstractmethod

from ..utils.logger import get_logger


class BaseProvider(ABC):
    """所有 Provider（LLM / Image）的公共基类。"""

    def __init__(self) -> None:
        self.logger = get_logger(self.__class__.__name__)

    @property
    @abstractmethod
    def name(self) -> str:
        """Provider 标识（供日志与响应中的 model_used 使用）。"""

    @abstractmethod
    async def health_check(self) -> bool:
        """健康检查：真实厂商探测远端，Mock 恒为 True。"""
