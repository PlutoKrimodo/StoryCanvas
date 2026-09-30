#!/usr/bin/env bash
# ============================================================
# Phase 4（M3 AI Service 基础：文本解析 + Prompt 优化）验收脚本
#
# 前置条件：
#   1. AI Service 已启动（uvicorn app.main:app --port 8000）
#   2. 已安装 curl 与 jq
#   3. 若配置了 INTERNAL_API_KEY，需通过 KEY=xxx 传入
#
# 用法：
#   ./scripts/verify_m3.sh
#   BASE=http://localhost:8000/api/v1 ./scripts/verify_m3.sh
#   KEY=your_internal_key ./scripts/verify_m3.sh
#
# 覆盖验收项（对应 docs/16 第 8.4 节 M3 验收标准）：
#   健康检查 / 风格列表 / 文本解析返回结构化数据 /
#   Prompt 优化（文本与结构化两种入参）/ 字段校验
# ============================================================
set -uo pipefail

BASE="${BASE:-http://localhost:8000/api/v1}"
KEY="${KEY:-}"
BODY_FILE="$(mktemp)"
trap 'rm -f "$BODY_FILE"' EXIT

PASS=0
FAIL=0

ok()  { printf '  \033[32m✅ %s\033[0m\n' "$1"; PASS=$((PASS + 1)); }
bad() { printf '  \033[31m❌ %s\033[0m\n' "$1"; FAIL=$((FAIL + 1)); }
warn() { printf '  \033[33m⚠ %s\033[0m\n' "$1"; }

expect() {
  if [ "$2" = "$3" ]; then ok "$1（$3）"; else bad "$1 → 期望 $2，实际 $3"; fi
}

request() {
  local method="$1" path="$2" data="${3:-}"
  local args=(-s -o "$BODY_FILE" -w '%{http_code}' -X "$method" "${BASE}${path}"
              -H 'Content-Type: application/json')
  [ -n "$KEY" ] && args+=(-H "X-Internal-Api-Key: ${KEY}")
  [ -n "$data" ] && args+=(-d "$data")
  curl "${args[@]}"
}

field() { jq -r "$1" "$BODY_FILE" 2>/dev/null; }
json()  { jq -nc "$@"; }

command -v curl >/dev/null || { echo "缺少 curl，请先安装"; exit 1; }
command -v jq   >/dev/null || { echo "缺少 jq，请先安装（sudo apt install jq）"; exit 1; }

printf '\n== M3 验收开始 | 目标: %s ==\n\n' "$BASE"

# ---------- 0. 健康检查 ----------
echo "[0] 健康检查"
code=$(request GET /health/)
expect "健康检查成功" 200 "$code"
expect "服务状态 ok" "ok" "$(field '.status')"
if [ -n "$(field '.providers.llm')" ]; then
  ok "LLM Provider = $(field '.providers.llm')"
else
  warn "未返回 providers.llm（旧版本 AI Service？）"
fi

# ---------- 1. 风格列表 ----------
echo "[1] 风格列表"
code=$(request GET /analysis/styles)
expect "风格列表成功" 200 "$code"
STYLE_COUNT="$(field '.styles | length')"
if [ "${STYLE_COUNT:-0}" -ge 5 ] 2>/dev/null; then
  ok "风格数量 ${STYLE_COUNT}"
else
  bad "风格数量异常（${STYLE_COUNT}）"
fi

# ---------- 2. 文本解析 ----------
echo "[2] 文本解析返回结构化数据"
STORY='小女孩在雨后的森林里发现了一只受伤的小鸟。'
code=$(request POST /analysis/parse "$(json --arg t "$STORY" '{text:$t}')")
expect "解析成功" 200 "$code"
if [ "$(field '.parsed_data | has("characters")')" = "true" ] \
   && [ "$(field '.parsed_data | has("objects")')" = "true" ] \
   && [ "$(field '.parsed_data | has("actions")')" = "true" ] \
   && [ "$(field '.parsed_data | has("style")')" = "true" ]; then
  ok "返回角色/物品/动作/风格字段"
else
  bad "结构化字段缺失"
fi
CHAR_COUNT="$(field '.parsed_data.characters | length')"
if [ "${CHAR_COUNT:-0}" -ge 1 ] 2>/dev/null; then
  ok "解析出 ${CHAR_COUNT} 个角色"
else
  bad "未解析出角色"
fi

echo "[3] 解析：空文本被拒"
code=$(request POST /analysis/parse "$(json '{text:""}')")
expect "空文本被拒" 422 "$code"

# ---------- 3. Prompt 优化 ----------
echo "[4] Prompt 优化（传文本）"
code=$(request POST /analysis/optimize "$(json --arg t "$STORY" '{text:$t,style:"watercolor"}')")
expect "优化成功" 200 "$code"
PROMPT="$(field '.prompt')"
if [ -n "$PROMPT" ] && [ "$PROMPT" != "null" ]; then ok "返回优化后 Prompt"; else bad "未返回 Prompt"; fi
if printf '%s' "$PROMPT" | grep -q 'watercolor'; then ok "Prompt 含水彩风格关键词"; else bad "Prompt 缺少风格关键词"; fi
if [ -n "$(field '.negative_prompt')" ]; then ok "返回负向 Prompt"; else bad "缺少负向 Prompt"; fi

echo "[5] Prompt 优化（传结构化数据）"
code=$(request POST /analysis/optimize \
  "$(json '{style:"cartoon",parsed_data:{characters:[{name:"小熊"}],scene:{location:"草地"},objects:["气球"],actions:["跑"],emotions:["开心"],style:{}}}')")
expect "优化成功" 200 "$code"
if printf '%s' "$(field '.prompt')" | grep -q '小熊'; then ok "Prompt 含角色名"; else bad "Prompt 缺少角色名"; fi

echo "[6] 优化：两种入参都缺失被拒"
code=$(request POST /analysis/optimize "$(json '{style:"cartoon"}')")
expect "缺少入参被拒" 422 "$code"

printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
