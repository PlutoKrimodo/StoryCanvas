"""BYOK 凭据测试 API。

仅由 C++ 后端在「保存并验证」时调用；请求体包含用户明文 Key，
因此：
  - 不记录请求体 / 请求头；
  - 异常只返回可读原因，不拼接厂商响应体；
  - 图像能力生成一张 1K 测试图做真实验证（会计费一次），测试图不落盘。
"""

from __future__ import annotations

import asyncio
import time

from fastapi import APIRouter, Depends

from ...models.schemas import (
    CredentialTestRequest,
    CredentialTestResponse,
    ImageCredential,
    ImageGenerationParams,
    LLMCredential,
)
from ...utils.exceptions import AIServiceException
from ...utils.url_safety import UnsafeUrlError, validate_public_https_url
from .deps import build_image_provider, build_llm_provider, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


@router.post("/test", response_model=CredentialTestResponse)
async def test_credential(request: CredentialTestRequest) -> CredentialTestResponse:
    started = time.perf_counter()

    def elapsed_ms() -> int:
        return int((time.perf_counter() - started) * 1000)

    if request.api_base:
        try:
            await asyncio.to_thread(validate_public_https_url, request.api_base)
        except UnsafeUrlError as exc:
            return CredentialTestResponse(
                ok=False,
                verified=False,
                latency_ms=elapsed_ms(),
                message=f"Base URL 不安全：{exc}",
            )

    try:
        if request.capability == "llm":
            provider = build_llm_provider(
                LLMCredential(
                    provider=request.provider,
                    api_key=request.api_key,
                    api_base=request.api_base,
                    model=request.model,
                )
            )
            # 1 token 真实探测：能验证 Key / Base URL / 模型三者是否可用
            await provider.generate(
                [{"role": "user", "content": "ping"}],
                temperature=0.0,
                max_tokens=1,
            )
            return CredentialTestResponse(
                ok=True,
                verified=True,
                latency_ms=elapsed_ms(),
                message="连接成功，API Key 可用",
            )

        if request.capability == "image":
            provider = build_image_provider(
                ImageCredential(
                    provider=request.provider,
                    api_key=request.api_key,
                    api_base=request.api_base,
                    model=request.model,
                )
            )
            # 真实生成一张 1K 测试图验证 Key；生成后丢弃，不落盘、不下载
            await provider.generate(
                ImageGenerationParams(
                    prompt="a single simple color swatch, one continuous image",
                    width=1024,
                    height=1024,
                    negative_prompt="text, watermark, multiple panels",
                )
            )
            return CredentialTestResponse(
                ok=True,
                verified=True,
                latency_ms=elapsed_ms(),
                message="连接成功，图像 API Key 可用（已生成 1 张测试图，会计费一次）",
            )

        return CredentialTestResponse(
            ok=False,
            verified=False,
            latency_ms=elapsed_ms(),
            message="capability 仅支持 llm 或 image",
        )
    except AIServiceException as exc:
        # exc.message 已由 Provider 归一为可读原因，且不含 Key
        return CredentialTestResponse(
            ok=False,
            verified=False,
            latency_ms=elapsed_ms(),
            message=exc.message,
        )
    except Exception:
        return CredentialTestResponse(
            ok=False,
            verified=False,
            latency_ms=elapsed_ms(),
            message="测试失败，请检查网络或稍后重试",
        )
