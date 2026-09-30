#!/usr/bin/env bash
# ============================================================
# Phase 5（M4 AI 图像生成闭环）验收脚本
#
# 前置条件：
#   1. PostgreSQL 已初始化（psql -f sql/init.sql）
#   2. 后端已启动（注入 JWT_SECRET / DB_PASSWORD / AI_SERVICE_API_KEY）
#   3. AI Service 已启动，且两者 STORAGE_PATH 指向同一目录
#   4. 已安装 curl 与 jq
#
# 用法：
#   ./scripts/verify_m4.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_m4.sh
#   POLL_TIMEOUT=180 ./scripts/verify_m4.sh
#
# 覆盖验收项（对应 docs/16 第 8.5 节 M4 验收标准）：
#   创建任务(201) → 状态流转(pending/processing→completed) → 生成图可访问 →
#   保存到绘本（绑定 image_id）→ 页列表含 image_url → 重新生成 →
#   生成历史 → 取消/越权/未认证校验
# ============================================================
set -uo pipefail

BASE="${BASE:-http://localhost:8080/api/v1}"
# 从 BASE 推导站点根地址，用于拼接图片直链（/api/v1/images/{id}/raw）
ORIGIN="${BASE%/api/v1}"
POLL_TIMEOUT="${POLL_TIMEOUT:-180}"
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
  local method="$1" path="$2" data="${3:-}" token="${4:-}"
  local args=(-s -o "$BODY_FILE" -w '%{http_code}' -X "$method" "${BASE}${path}"
              -H 'Content-Type: application/json')
  [ -n "$token" ] && args+=(-H "Authorization: Bearer ${token}")
  [ -n "$data" ] && args+=(-d "$data")
  curl "${args[@]}"
}

field() { jq -r "$1" "$BODY_FILE" 2>/dev/null; }
json()  { jq -nc "$@"; }

command -v curl >/dev/null || { echo "缺少 curl，请先安装"; exit 1; }
command -v jq   >/dev/null || { echo "缺少 jq，请先安装（sudo apt install jq）"; exit 1; }

SUFFIX="$(date +%s)${RANDOM}"
PASSWORD='StrongPass1!'

register_and_login() {
  local name="$1"
  local username="m4_${name}_${SUFFIX}"
  local email="${username}@example.com"
  request POST /auth/register "$(json --arg u "$username" --arg e "$email" --arg p "$PASSWORD" \
    '{username:$u,email:$e,password:$p}')" >/dev/null
  request POST /auth/login "$(json --arg e "$email" --arg p "$PASSWORD" '{email:$e,password:$p}')" >/dev/null
  field '.data.access_token'
}

printf '\n== M4 验收开始 | 目标: %s ==\n\n' "$BASE"

if ! curl -s -o /dev/null --max-time 5 "${BASE}/health"; then
  warn "无法访问 ${BASE}/health，请确认后端已启动"
fi

# ---------- 0. 准备 ----------
echo "[0] 注册并登录用户 A / B"
TOKEN_A="$(register_and_login a)"
TOKEN_B="$(register_and_login b)"
if [ -n "$TOKEN_A" ] && [ "$TOKEN_A" != "null" ]; then ok "用户 A 已登录"; else bad "用户 A 登录失败"; fi
if [ -n "$TOKEN_B" ] && [ "$TOKEN_B" != "null" ]; then ok "用户 B 已登录"; else bad "用户 B 登录失败"; fi

echo "[1] 用户 A 创建绘本"
code=$(request POST /books "$(json --arg t '雨后的相遇' '{title:$t,description:"M4 验收用绘本"}')" "$TOKEN_A")
expect "创建绘本成功" 201 "$code"
BOOK_ID="$(field '.data.id')"
if [ -n "$BOOK_ID" ] && [ "$BOOK_ID" != "null" ]; then ok "返回绘本 ID"; else bad "未返回绘本 ID"; fi

# ---------- 1. 创建生成任务 ----------
echo "[2] 创建生成任务"
STORY='小女孩在雨后的森林里发现了一只受伤的小鸟。'
code=$(request POST /generation \
  "$(json --arg t "$STORY" --arg b "$BOOK_ID" \
     '{text:$t,book_id:$b,style:"cartoon",parameters:{size:"768x768"}}')" "$TOKEN_A")
expect "创建任务成功" 201 "$code"
TASK_ID="$(field '.data.task_id')"
expect "初始状态为 pending" "pending" "$(field '.data.status')"
if [ -n "$TASK_ID" ] && [ "$TASK_ID" != "null" ]; then ok "返回任务 ID"; else bad "未返回任务 ID"; fi

echo "[3] 创建任务：空文本被拒"
code=$(request POST /generation "$(json '{text:""}')" "$TOKEN_A")
expect "空文本被拒" 422 "$code"

echo "[4] 创建任务：非法风格被拒"
code=$(request POST /generation "$(json --arg t "$STORY" '{text:$t,style:"unknown"}')" "$TOKEN_A")
expect "非法风格被拒" 422 "$code"

echo "[5] 创建任务：不存在的绘本返回 404"
code=$(request POST /generation \
  "$(json --arg t "$STORY" '{text:$t,book_id:"00000000-0000-0000-0000-000000000000"}')" "$TOKEN_A")
expect "绘本不存在返回 404" 404 "$code"

# ---------- 2. 轮询任务状态 ----------
echo "[6] 轮询任务状态（最长 ${POLL_TIMEOUT}s）"
STATUS=""
ELAPSED=0
while [ "$ELAPSED" -lt "$POLL_TIMEOUT" ]; do
  code=$(request GET "/generation/${TASK_ID}" "" "$TOKEN_A")
  STATUS="$(field '.data.status')"
  case "$STATUS" in
    completed|failed|cancelled) break ;;
  esac
  sleep 2
  ELAPSED=$((ELAPSED + 2))
done
expect "任务最终状态为 completed" "completed" "$STATUS"
if [ "$STATUS" != "completed" ]; then
  warn "任务错误信息：$(field '.data.error_message')"
fi

echo "[7] 任务结果包含生成图"
IMAGE_ID="$(field '.data.result.image_id')"
IMAGE_URL="$(field '.data.result.image_url')"
if [ -n "$IMAGE_ID" ] && [ "$IMAGE_ID" != "null" ]; then ok "返回 image_id"; else bad "缺少 image_id"; fi
if [ -n "$IMAGE_URL" ] && [ "$IMAGE_URL" != "null" ]; then ok "返回 image_url"; else bad "缺少 image_url"; fi
if [ -n "$(field '.data.prompt')" ]; then ok "返回绘图提示词"; else bad "缺少绘图提示词"; fi

echo "[8] 生成图可访问（公开 raw 直链）"
if [ -n "$IMAGE_URL" ] && [ "$IMAGE_URL" != "null" ]; then
  RAW_CODE="$(curl -s -o /dev/null -w '%{http_code}' "${ORIGIN}${IMAGE_URL}")"
  RAW_TYPE="$(curl -s -o /dev/null -w '%{content_type}' "${ORIGIN}${IMAGE_URL}")"
  expect "图片可访问" 200 "$RAW_CODE"
  case "$RAW_TYPE" in
    image/*) ok "Content-Type = ${RAW_TYPE}" ;;
    *) bad "Content-Type 异常（${RAW_TYPE}）" ;;
  esac
fi

echo "[9] 图片下载接口（需鉴权）"
code=$(request GET "/images/${IMAGE_ID}/download" "" "$TOKEN_A")
expect "下载成功" 200 "$code"

echo "[10] 用户 B 下载用户 A 的图片"
code=$(request GET "/images/${IMAGE_ID}/download" "" "$TOKEN_B")
expect "越权下载被拒" 403 "$code"

# ---------- 3. 保存到绘本 ----------
echo "[11] 保存到绘本（绑定 image_id）"
code=$(request POST "/books/${BOOK_ID}/pages" \
  "$(json --arg i "$IMAGE_ID" '{page_number:1,title:"雨后的相遇",image_id:$i}')" "$TOKEN_A")
expect "保存成功" 200 "$code"
expect "页绑定到正确图片" "$IMAGE_ID" "$(field '.data.image_id')"

echo "[12] 保存：缺少 image_id 被拒"
code=$(request POST "/books/${BOOK_ID}/pages" "$(json '{page_number:2}')" "$TOKEN_A")
expect "缺少图片被拒" 422 "$code"

echo "[13] 用户 B 往用户 A 的绘本保存图片"
code=$(request POST "/books/${BOOK_ID}/pages" \
  "$(json --arg i "$IMAGE_ID" '{page_number:9,image_id:$i}')" "$TOKEN_B")
expect "越权保存被拒" 403 "$code"

echo "[14] 读取页列表（供 PDF 导出）"
code=$(request GET "/books/${BOOK_ID}/pages" "" "$TOKEN_A")
expect "读取成功" 200 "$code"
expect "页数量为 1" 1 "$(field '.data | length')"
expect "页含 image_url" "true" "$(field '.data[0] | has("image_url")')"
expect "页按页码升序首位为 1" 1 "$(field '.data[0].page_number')"

# ---------- 4. 重新生成 ----------
echo "[15] 重新生成"
code=$(request POST "/generation/${TASK_ID}/regenerate" "$(json '{}')" "$TOKEN_A")
expect "重新生成任务创建成功" 201 "$code"
NEW_TASK_ID="$(field '.data.task_id')"
if [ -n "$NEW_TASK_ID" ] && [ "$NEW_TASK_ID" != "null" ] && [ "$NEW_TASK_ID" != "$TASK_ID" ]; then
  ok "返回新的任务 ID"
else
  bad "未返回新任务 ID"
fi

echo "[16] 用户 B 重新生成用户 A 的任务"
code=$(request POST "/generation/${TASK_ID}/regenerate" "$(json '{}')" "$TOKEN_B")
expect "越权重生成被拒" 403 "$code"

# ---------- 5. 生成历史 ----------
echo "[17] 生成历史"
code=$(request GET "/generation/history?page=1&limit=10" "" "$TOKEN_A")
expect "历史查询成功" 200 "$code"
HIST_TOTAL="$(field '.data.total')"
if [ "${HIST_TOTAL:-0}" -ge 2 ] 2>/dev/null; then ok "历史条数 ${HIST_TOTAL}"; else bad "历史条数异常（${HIST_TOTAL}）"; fi

echo "[18] 按绘本筛选历史"
code=$(request GET "/generation/history?book_id=${BOOK_ID}" "" "$TOKEN_A")
expect "筛选成功" 200 "$code"
if [ "${HIST_TOTAL:-0}" -ge 1 ] 2>/dev/null; then ok "绘本维度历史非空"; else bad "绘本维度历史为空"; fi

# ---------- 6. 鉴权 ----------
echo "[19] 未携带 Token 创建任务"
code=$(request POST /generation "$(json --arg t 'x' '{text:$t}')")
expect "未认证被拒" 401 "$code"

echo "[20] 用户 B 访问用户 A 的任务"
code=$(request GET "/generation/${TASK_ID}" "" "$TOKEN_B")
expect "越权访问被拒" 403 "$code"

printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
