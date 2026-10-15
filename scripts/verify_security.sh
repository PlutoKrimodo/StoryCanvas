#!/usr/bin/env bash
# ============================================================
# 安全加固验收脚本（Q3 / Q5 / Q8）
#
# 前置条件：
#   1. 数据库已初始化；后端与 AI 服务已启动（AI 服务 mock 模式即可）
#   2. 已安装 curl 与 jq
#
# 用法：
#   ./scripts/verify_security.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_security.sh
#   # Q8 超限文案（AI 服务需以 MAX_IMAGE_SIZE=1000 之类的极小上限启动）：
#   VERIFY_OVERSIZE=1 ./scripts/verify_security.sh
#
# 覆盖验收项：
#   Q3② 游客 Token 调写接口（创建/更新/删除绘本、保存页）→ 403；只读接口不受影响
#   Q3① POST /auth/guest 按 IP 限频（缺省 10 次/分）→ 连续调用在 12 次内出现 429
#   Q5  匿名请求按 IP 限频（缺省 100 次/分）→ 150 次内出现 429，且不早于第 1 次；
#       认证请求按用户分桶 → 匿名额度耗尽后仍可正常访问
#   Q8  （可选）超限图失败原因透传：任务 failed 且 error_message 含「大小上限」
#
# 注意：
#   - 限频按刷新界定时，本脚本会消耗本机 IP 的匿名额度；建议最后运行本脚本，
#     并在运行后 1 分钟内避免继续跑其他验收脚本；
#   - 若调整过 .env 中 RATE_LIMIT_* 配置，脚本中的次数上限断言需同步调整。
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
EMAIL="security_${SUFFIX}@example.com"
USERNAME="security_${SUFFIX}"
FAKE_BOOK_ID="00000000-0000-0000-0000-000000000000"

printf '\n== 安全加固验收（Q3/Q5/Q8）| 目标: %s ==\n\n' "$BASE"

# ---------- 1. 正式用户写接口（对照组） ----------
echo "[1] 正式用户创建绘本（对照组，确保 MemberFilter 不误伤）"
code=$(request POST /auth/register \
  "$(json --arg u "$USERNAME" --arg e "$EMAIL" --arg p "$PASSWORD" \
     '{username:$u,email:$e,password:$p}')")
expect "注册成功" 201 "$code"
USER_TOKEN="$(field '.data.access_token')"
if [ -z "$USER_TOKEN" ] || [ "$USER_TOKEN" = "null" ]; then
  bad "未获取到用户 Token，后续用例无法继续"
  printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
  exit 1
fi

code=$(request POST /books "$(json '{title:"安全验收绘本",style:"cartoon"}')" "$USER_TOKEN")
expect "正式用户创建绘本成功" 201 "$code"
BOOK_ID="$(field '.data.book_id')"

# ---------- 2. Q3② 游客写接口拦截 ----------
echo "[2] 游客 Token 调用写接口 → 一律 403（Q3②）"
code=$(request POST /auth/guest)
expect "游客会话创建成功" 200 "$code"
GUEST_TOKEN="$(field '.data.guest_token')"
if [ -z "$GUEST_TOKEN" ] || [ "$GUEST_TOKEN" = "null" ]; then
  bad "未获取到游客 Token，后续用例无法继续"
  printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
  exit 1
fi

code=$(request POST /books "$(json '{title:"游客不该出现的绘本"}')" "$GUEST_TOKEN")
expect "游客创建绘本被拒（403）" 403 "$code"
if printf '%s' "$(field '.message')" | grep -q '注册或登录'; then
  ok "403 文案包含登录引导"
else
  warn "403 文案未包含登录引导：$(field '.message')"
fi

code=$(request PUT "/books/${BOOK_ID}" "$(json '{title:"游客改不动"}')" "$GUEST_TOKEN")
expect "游客更新绘本被拒（403）" 403 "$code"

code=$(request DELETE "/books/${BOOK_ID}" "" "$GUEST_TOKEN")
expect "游客删除绘本被拒（403）" 403 "$code"

code=$(request POST "/books/${BOOK_ID}/pages" "$(json '{page_number:1}')" "$GUEST_TOKEN")
expect "游客保存页被拒（403）" 403 "$code"

# 只读接口不受影响（游客浏览自己的生成历史）
code=$(request GET "/generation/history?page=1&limit=1" "" "$GUEST_TOKEN")
expect "游客只读接口不受影响（200）" 200 "$code"

# ---------- 3. Q3① 游客会话签发按 IP 限频 ----------
echo "[3] POST /auth/guest 连续调用 → 12 次内触发 429（Q3①，缺省 10 次/分）"
GUEST_429_AT=0
for i in $(seq 1 12); do
  code=$(request POST /auth/guest)
  if [ "$code" = "429" ]; then GUEST_429_AT=$i; break; fi
done
if [ "$GUEST_429_AT" -gt 0 ]; then
  ok "第 ${GUEST_429_AT} 次调用触发 429"
else
  bad "12 次连续调用未触发 429（检查 RATE_LIMIT_GUEST_TOKEN_PER_MIN）"
fi

# ---------- 4. Q5 匿名额度（按 IP） ----------
echo "[4] 匿名请求连发 → 150 次内出现 429（Q5，缺省 100 次/分）"
ANON_429_AT=0
for i in $(seq 1 150); do
  code=$(request GET /users/me)   # 无 Token：正常应为 401，超限则为 429
  if [ "$code" = "429" ]; then ANON_429_AT=$i; break; fi
done
if [ "$ANON_429_AT" -gt 0 ] && [ "$ANON_429_AT" -le 101 ]; then
  ok "匿名请求第 ${ANON_429_AT} 次触发 429（≤ 101）"
elif [ "$ANON_429_AT" -gt 101 ]; then
  bad "429 来得太晚：第 ${ANON_429_AT} 次（高于配置的 100 次/分 + 1）"
else
  bad "150 次内未触发 429（检查 RATE_LIMIT_ANON_PER_MIN 与 advice 是否注册）"
fi

echo "[5] 认证请求按用户分桶：匿名额度耗尽后不受影响"
code=$(request GET /users/me "" "$USER_TOKEN")
expect "匿名额度耗尽后，认证请求仍 200" 200 "$code"

# ---------- 5. Q8（可选）超限图文案 ----------
if [ "${VERIFY_OVERSIZE:-0}" = "1" ]; then
  echo "[6] 超限图失败原因透传（Q8，需 AI 服务以极小 MAX_IMAGE_SIZE 启动）"
  STORY='小女孩在雨后的森林里发现了一只受伤的小鸟。'
  code=$(request POST /generation \
    "$(json --arg t "$STORY" '{text:$t,style:"cartoon",parameters:{size:"768x768"}}')" \
    "$USER_TOKEN")
  expect "创建任务成功" 201 "$code"
  TASK_ID="$(field '.data.task_id')"

  STATUS=""
  ELAPSED=0
  while [ "$ELAPSED" -lt "$POLL_TIMEOUT" ]; do
    request GET "/generation/${TASK_ID}" "" "$USER_TOKEN" >/dev/null
    STATUS="$(field '.data.status')"
    case "$STATUS" in completed|failed|cancelled) break ;; esac
    sleep 2; ELAPSED=$((ELAPSED + 2))
  done
  expect "任务最终状态 failed" "failed" "$STATUS"

  MSG="$(field '.data.error_message')"
  if printf '%s' "$MSG" | grep -q '大小上限'; then
    ok "error_message 含「大小上限」：${MSG}"
  else
    bad "error_message 未含「大小上限」：${MSG:-（空）}"
  fi
else
  echo "[6] 超限图文案：跳过（设 VERIFY_OVERSIZE=1 且 AI 服务以 MAX_IMAGE_SIZE=1000 启动时执行）"
fi

# ---------- 汇总 ----------
printf '\n== 结果：通过 %d，失败 %d ==\n' "$PASS" "$FAIL"
printf '== 提示：限频窗口为 60s，建议等待 1 分钟后再运行其他验收脚本 ==\n\n'
[ "$FAIL" -eq 0 ]
