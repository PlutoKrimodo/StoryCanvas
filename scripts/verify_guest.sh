#!/usr/bin/env bash
# ============================================================
# T-1 Guest-first（能力层）验收脚本
#
# 前置条件：
#   1. 数据库已初始化（且已执行 sql/migrate_guest.sql 增加 is_guest 列）
#   2. 后端与 AI 服务已启动（AI 服务建议 mock 模式以加速）
#   3. 已安装 curl 与 jq
#
# 用法：
#   ./scripts/verify_guest.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_guest.sh
#
# 覆盖验收项（对应 docs/TODO.md §10 能力层验收标准）：
#   未登录创建游客会话 → 游客生成 1 张 → 超额返回 429 →
#   登录认领 → 生成历史归属转移 → 游客 Token 失效
# ============================================================
set -uo pipefail

BASE="${BASE:-http://localhost:8080/api/v1}"
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
EMAIL="guestflow_${SUFFIX}@example.com"
USERNAME="guestflow_${SUFFIX}"

printf '\n== T-1 Guest-first 验收开始 | 目标: %s ==\n\n' "$BASE"

# ---------- 1. 创建游客会话 ----------
echo "[1] 创建游客会话"
code=$(request POST /auth/guest)
expect "游客会话创建成功" 200 "$code"
GUEST_TOKEN="$(field '.data.guest_token')"
GUEST_ID="$(field '.data.guest_id')"
QUOTA="$(field '.data.quota')"
if [ -n "$GUEST_TOKEN" ] && [ "$GUEST_TOKEN" != "null" ]; then ok "返回游客 Token"; else bad "未返回游客 Token"; fi
if [ -n "$GUEST_ID" ] && [ "$GUEST_ID" != "null" ]; then ok "返回 guest_id"; else bad "未返回 guest_id"; fi
if [ "${QUOTA:-0}" -ge 1 ] 2>/dev/null; then ok "游客额度 = ${QUOTA}"; else warn "未返回额度字段"; fi

# ---------- 2. 游客生成（不登录） ----------
echo "[2] 游客（未登录）创建生成任务"
STORY='小女孩在雨后的森林里发现了一只受伤的小鸟。'
code=$(request POST /generation \
  "$(json --arg t "$STORY" '{text:$t,style:"cartoon",parameters:{size:"768x768"}}')" "$GUEST_TOKEN")
expect "游客创建任务成功" 201 "$code"
TASK_ID="$(field '.data.task_id')"

echo "[3] 轮询游客任务至完成"
STATUS=""
ELAPSED=0
while [ "$ELAPSED" -lt "$POLL_TIMEOUT" ]; do
  request GET "/generation/${TASK_ID}" "" "$GUEST_TOKEN" >/dev/null
  STATUS="$(field '.data.status')"
  case "$STATUS" in completed|failed|cancelled) break ;; esac
  sleep 2; ELAPSED=$((ELAPSED + 2))
done
expect "游客任务最终状态 completed" "completed" "$STATUS"
IMAGE_ID="$(field '.data.result.image_id')"
if [ -n "$IMAGE_ID" ] && [ "$IMAGE_ID" != "null" ]; then ok "游客生成图已落库（image_id）"; else bad "缺少 image_id"; fi

# ---------- 3. 超额 429 ----------
echo "[4] 游客再次生成 → 应返回 429"
code=$(request POST /generation \
  "$(json --arg t "$STORY" '{text:$t,style:"watercolor"}')" "$GUEST_TOKEN")
expect "超额返回 429" 429 "$code"
if printf '%s' "$(field '.message')" | grep -q '额度'; then ok "429 提示额度用尽"; else warn "429 文案未见「额度」"; fi

# ---------- 4. 登录认领 ----------
echo "[5] 注册新用户并认领游客资源"
code=$(request POST /auth/register \
  "$(json --arg u "$USERNAME" --arg e "$EMAIL" --arg p "$PASSWORD" --arg g "$GUEST_ID" \
     '{username:$u,email:$e,password:$p,guest_id:$g}')")
expect "注册成功" 201 "$code"
USER_TOKEN="$(field '.data.access_token')"
if [ -n "$USER_TOKEN" ] && [ "$USER_TOKEN" != "null" ]; then ok "返回用户 Token"; else bad "未返回用户 Token"; fi

echo "[6] 认领后：用户生成历史包含游客的任务"
code=$(request GET "/generation/history?page=1&limit=10" "" "$USER_TOKEN")
expect "历史查询成功" 200 "$code"
HAS_TASK="$(field ".data.items[] | select(.task_id == \"${TASK_ID}\") | .task_id")"
if [ -n "$HAS_TASK" ]; then ok "游客任务已归属到真实用户"; else bad "游客任务未认领"; fi

echo "[7] 认领后：游客 Token 已失效"
code=$(request GET "/generation/${TASK_ID}" "" "$GUEST_TOKEN")
expect "旧游客 Token 被拒" 401 "$code"

# ---------- 5. 汇总 ----------
printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
