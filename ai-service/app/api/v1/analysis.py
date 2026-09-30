"""文本分析 API（Phase 4）。

端点命名按 docs/16 §7.5 统一为 `analysis/parse` 与 `analysis/optimize`。
"""

from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException

from ...models.schemas import (
    AnalysisResult,
    PromptOptimizeRequest,
    PromptOptimizeResponse,
    TextAnalysisRequest,
)
from ...prompts.styles import list_styles
from ...services.prompt_optimizer import PromptOptimizerService
from ...services.text_analyzer import TextAnalyzerService
from .deps import get_prompt_optimizer, get_text_analyzer, verify_internal_key

router = APIRouter(dependencies=[Depends(verify_internal_key)])


@router.get("/styles")
async def get_styles() -> dict:
    """列出支持的艺术风格（供前端 / 文档手工调用）。"""
    return {"styles": list_styles()}


@router.post("/parse", response_model=AnalysisResult)
async def parse_text(
    request: TextAnalysisRequest,
    analyzer: TextAnalyzerService = Depends(get_text_analyzer),
) -> AnalysisResult:
    """把故事文本解析为结构化数据（角色 / 场景 / 物品 / 动作 / 风格）。"""
    return await analyzer.analyze(request.text)


@router.post("/optimize", response_model=PromptOptimizeResponse)
async def optimize_prompt(
    request: PromptOptimizeRequest,
    analyzer: TextAnalyzerService = Depends(get_text_analyzer),
    optimizer: PromptOptimizerService = Depends(get_prompt_optimizer),
) -> PromptOptimizeResponse:
    """把结构化数据（或文本）优化为最终绘图 Prompt。"""
    if request.parsed_data is not None:
        parsed = request.parsed_data
    elif request.text:
        parsed = (await analyzer.analyze(request.text)).parsed_data
    else:
        raise HTTPException(status_code=422, detail="text 与 parsed_data 至少提供一个")

    prompt = optimizer.optimize(parsed, request.style, request.additional_params)
    return PromptOptimizeResponse(
        prompt=prompt,
        negative_prompt=optimizer.negative(request.style),
        style=request.style,
        parsed_data=parsed,
    )
