"""服务层基类。"""

from __future__ import annotations

from ..utils.logger import get_logger


class BaseService:
    """所有业务服务的公共基类：统一提供模块级 logger。"""

    def __init__(self) -> None:
        self.logger = get_logger(self.__class__.__name__)
