import logging
import sys

_CONFIGURED = False


def setup_logging(level: int = logging.INFO) -> None:
    """配置全局日志（幂等，可重复调用）。

    约束（对照 docs/16 §11.4）：密钥、Token、请求体中的敏感字段一律不得落日志，
    因此本模块只做格式统一，不提供打印请求体的便捷方法。
    """
    global _CONFIGURED
    if _CONFIGURED:
        return

    root = logging.getLogger()
    root.setLevel(level)

    # 避免 uvicorn / pytest 重复追加 handler
    if not any(isinstance(h, logging.StreamHandler) for h in root.handlers):
        handler = logging.StreamHandler(sys.stdout)
        handler.setFormatter(
            logging.Formatter(
                fmt="%(asctime)s | %(levelname)-7s | %(name)s | %(message)s",
                datefmt="%Y-%m-%d %H:%M:%S",
            )
        )
        root.addHandler(handler)

    _CONFIGURED = True


def get_logger(name: str) -> logging.Logger:
    """获取模块级 logger。"""
    return logging.getLogger(name)
