#!/usr/bin/env bash
# ============================================================
# 对话式多图生成验收脚本
#
# 前置条件：
#   1. 数据库已执行 sql/init.sql 或 sql/migrate_conversation.sql
#   2. 后端 / AI Service 已启动；演示可用 LLM_PROVIDER=mock IMAGE_PROVIDER=mock
#   3. 已安装 curl 与 jq
#
# 覆盖：分镜 → 逐格编辑 → 批次生成 → 会话回放 → 整组入绘本 → 游客限制
#
# 用法：
#   ./scripts/verify_chat.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_chat.sh
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
warn(){ printf '  \033[33m⚠ %s\033[0m\n' "$1"; }
expect() { if [ "$2" = "$3" ]; then ok "$1（$3）"; else bad "$1 → 期望 $2，实际 $3"; fi; }

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

command -v curl >/dev/null || { echo "缺少 curl"; exit 1; }
command -v jq   >/dev/null || { echo "缺少 jq"; exit 1; }

SUFFIX="$(date +%s)${RANDOM}"
USERNAME="chat_${SUFFIX}"
EMAIL="${USERNAME}@example.com"
PASSWORD='StrongPass1!'
STORY='小女孩在雨后的森林里发现一只受伤的小鸟。她把小鸟捧在手心带回家。第二天，小鸟在窗边唱起了歌。'

printf '\n== 对话式生成验收开始 | %s ==\n\n' "$BASE"

# ---------- 0. 身份 ----------
echo "[0] 注册并登录"
request POST /auth/register "$(json --arg u "$USERNAME" --arg e "$EMAIL" --arg p "$PASSWORD" \
  '{username:$u,email:$e,password:$p}')" >/dev/null
request POST /auth/login "$(json --arg e "$EMAIL" --arg p "$PASSWORD" '{email:$e,password:$p}')" >/dev/null
TOKEN="$(field '.data.access_token')"
if [ -n "$TOKEN" ] && [ "$TOKEN" != "null" ]; then ok "登录成功"; else bad "登录失败"; exit 1; fi

# ---------- 1. 分镜 ----------
echo "[1] 生成分镜（3 格）"
STATUS="$(request POST /generation/plans \
  "$(json --arg t "$STORY" '{text:$t,image_count:3,style:"watercolor",size:"1024x1024"}')" "$TOKEN")"
expect "分镜接口返回 201" 201 "$STATUS"
PLAN_ID="$(field '.data.plan_id')"
CONV_ID="$(field '.data.conversation_id')"
if [ "$(field '.data.panels | length')" = "3" ]; then ok "返回 3 格分镜"; else bad "分镜数量不为 3"; fi
if [ -n "$CONV_ID" ] && [ "$CONV_ID" != "null" ]; then ok "登录用户会话已创建"; else bad "缺少会话 ID"; fi

# ---------- 2. 逐格编辑 ----------
echo "[2] 编辑第 1 格"
PANELS="$(jq -c '.data.panels | .[0].text = "雨后的森林，小女孩蹲下发现受伤的小鸟" | map({index,text})' "$BODY_FILE")"
STATUS="$(request PUT "/generation/plans/${PLAN_ID}" "$(json --argjson p "$PANELS" '{panels:$p}')" "$TOKEN")"
expect "保存分镜返回 200" 200 "$STATUS"

# ---------- 3. 启动批次 ----------
echo "[3] 启动生成"
STATUS="$(request POST "/generation/batches/${PLAN_ID}/start" '{}' "$TOKEN")"
expect "启动返回 201" 201 "$STATUS"
if [ "$(field '.data.panel_count')" = "3" ]; then ok "批次包含 3 格"; else bad "批次格数异常"; fi

# ---------- 4. 轮询 ----------
echo "[4] 等待批次完成"
START=$SECONDS
FINAL=""
while [ $((SECONDS - START)) -lt "$POLL_TIMEOUT" ]; do
  request GET "/generation/batches/${PLAN_ID}" '' "$TOKEN" >/dev/null
  FINAL="$(field '.data.status')"
  case "$FINAL" in
    completed|partial|failed|cancelled) break ;;
  esac
  sleep 2
done
if [ "$FINAL" = "completed" ]; then
  ok "批次完成"
elif [ "$FINAL" = "partial" ]; then
  warn "批次部分完成（可检查单格错误）"
else
  bad "批次未完成：${FINAL:-timeout}"
fi
COMPLETED="$(jq -r '[.data.items[] | select(.status=="completed" and .image_id != null)] | length' "$BODY_FILE")"
if [ "$COMPLETED" -gt 0 ]; then ok "已生成 ${COMPLETED} 张图"; else bad "没有生成成功的图片"; fi

# ---------- 5. 会话回放 ----------
echo "[5] 会话回放"
STATUS="$(request GET "/conversations/${CONV_ID}" '' "$TOKEN")"
expect "会话详情返回 200" 200 "$STATUS"
MSG_COUNT="$(jq -r '.data.messages | length' "$BODY_FILE")"
if [ "$MSG_COUNT" -ge 2 ]; then ok "会话包含 ${MSG_COUNT} 条消息"; else bad "会话消息不足"; fi

# ---------- 6. 整组入绘本 ----------
echo "[6] 整组加入绘本"
request POST /books "$(json --arg t "对话验收绘本 ${SUFFIX}" '{title:$t,description:"verify_chat"}')" "$TOKEN" >/dev/null
BOOK_ID="$(field '.data.id')"
if [ -n "$BOOK_ID" ] && [ "$BOOK_ID" != "null" ]; then ok "绘本已创建"; else bad "绘本创建失败"; fi
request GET "/generation/batches/${PLAN_ID}" '' "$TOKEN" >/dev/null
IMAGE_IDS="$(jq -c '[.data.items[] | select(.image_id != null) | .image_id]' "$BODY_FILE")"
STATUS="$(request POST "/books/${BOOK_ID}/pages/batch" "$(json --argjson ids "$IMAGE_IDS" '{image_ids:$ids}')" "$TOKEN")"
expect "批量入绘本返回 200" 200 "$STATUS"
PAGE_COUNT="$(jq -r '.data.items | length' "$BODY_FILE")"
if [ "$PAGE_COUNT" -gt 0 ]; then ok "写入 ${PAGE_COUNT} 页"; else bad "未写入页面"; fi

# ---------- 7. 游客限制 ----------
echo "[7] 游客限制"
request POST /auth/guest '{}' >/dev/null
GUEST_TOKEN="$(field '.data.guest_token')"
STATUS="$(request GET /conversations '' "$GUEST_TOKEN")"
expect "游客访问会话返回 403" 403 "$STATUS"
STATUS="$(request POST /generation/plans \
  "$(json --arg t '一只小猫' '{text:$t,image_count:6,style:"cartoon",size:"1024x1024"}')" "$GUEST_TOKEN")"
expect "游客分镜接口可用" 201 "$STATUS"
if [ "$(field '.data.panel_count')" = "1" ]; then ok "游客强制 1 张"; else bad "游客张数未被限制"; fi

printf '\n== 结果：%d 通过 / %d 失败 ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
