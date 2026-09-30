"""通义万相（阿里云百炼）图像生成 Provider（真实实现，演示备用）。

DashScope 的文生图是**异步任务式**接口：
1. `POST /api/v1/services/aigc/text2image/image-synthesis`（`X-DashScope-Async: enable`）→ 返回 task_id
2. 轮询 `GET /api/v1/tasks/{task_id}` 直到 SUCCEEDED / FAILED
"""

from __future__ import annotations

import asyncio
import time
from typing import Dict, Optional

from ...models.schemas import ImageGenerationParams, ImageGenerationResult
from ...utils.config import settings
from ...utils.exceptions import ImageProviderException
from ...utils.http_client import get_json, post_json
from .base import ImageProvider

# 轮询间隔与总超时（秒）
_POLL_INTERVAL = 3.0
_TASK_TOTAL_TIMEOUT = 150.0


class WanxProvider(ImageProvider):
    """通义万相实现（提交 + 轮询）。"""

    def __init__(self, api_key: Optional[str] = None, model: Optional[str] = None) -> None:
        super().__init__()
        self.api_key = api_key if api_key is not None else settings.DASHSCOPE_API_KEY
        self.api_base = settings.DASHSCOPE_API_BASE.rstrip("/")
        self.model = model or settings.WANX_IMAGE_MODEL

    @property
    def name(self) -> str:
        return f"wanx:{self.model}"

    async def generate(self, params: ImageGenerationParams) -> ImageGenerationResult:
        if not self.api_key:
            raise ImageProviderException("DASHSCOPE_API_KEY 未配置，无法调用通义万相", 503)

        task_id = await self._submit(params)
        url = await self._poll(task_id)
        return ImageGenerationResult(
            image_url=url,
            width=params.width,
            height=params.height,
            seed=params.seed,
            finish_reason="success",
        )

    async def _submit(self, params: ImageGenerationParams) -> str:
        payload: Dict[str, object] = {
            "model": self.model,
            "input": {"prompt": params.prompt},
            "parameters": {
                "size": f"{params.width}*{params.height}",
                "n": 1,
            },
        }
        if params.negative_prompt:
            payload["input"]["negative_prompt"] = params.negative_prompt  # type: ignore[index]

        try:
            data = await post_json(
                f"{self.api_base}/api/v1/services/aigc/text2image/image-synthesis",
                headers={
                    "Authorization": f"Bearer {self.api_key}",
                    "Content-Type": "application/json",
                    "X-DashScope-Async": "enable",
                },
                payload=payload,
                timeout=settings.LLM_TIMEOUT,
            )
        except Exception as exc:
            self.logger.error("通义万相提交失败: %s", exc)
            raise ImageProviderException(f"通义万相提交失败: {exc}", 502) from exc

        task_id = (data.get("output") or {}).get("task_id")  # type: ignore[union-attr]
        if not task_id:
            raise ImageProviderException(f"通义万相未返回 task_id: {data}", 502)
        return str(task_id)

    async def _poll(self, task_id: str) -> str:
        deadline = time.monotonic() + _TASK_TOTAL_TIMEOUT
        while time.monotonic() < deadline:
            try:
                data = await get_json(
                    f"{self.api_base}/api/v1/tasks/{task_id}",
                    headers={"Authorization": f"Bearer {self.api_key}"},
                    timeout=settings.LLM_TIMEOUT,
                )
            except Exception as exc:
                self.logger.warning("通义万相轮询失败，将重试: %s", exc)
                await asyncio.sleep(_POLL_INTERVAL)
                continue

            output = data.get("output") or {}
            status = output.get("task_status")
            if status == "SUCCEEDED":
                results = output.get("results") or []
                if results and results[0].get("url"):
                    return str(results[0]["url"])
                raise ImageProviderException(f"通义万相成功但无图片: {data}", 502)
            if status == "FAILED":
                raise ImageProviderException(
                    f"通义万相生成失败: {output.get('message', data)}", 502
                )
            await asyncio.sleep(_POLL_INTERVAL)

        raise ImageProviderException("通义万相生成超时", 504)

    async def health_check(self) -> bool:
        return bool(self.api_key)
