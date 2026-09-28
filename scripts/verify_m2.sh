#!/usr/bin/env bash
# ============================================================
# Phase 3（M2 绘本项目管理）验收脚本
#
# 前置条件：
#   1. PostgreSQL 已初始化（psql -f sql/init.sql）
#   2. 后端已启动，且 JWT_SECRET / DB_PASSWORD 等已通过环境变量注入
#   3. 已安装 curl 与 jq
#
# 用法：
#   ./scripts/verify_m2.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_m2.sh
#
# 覆盖验收项（对应 docs/16 第 8.3 节 M2 验收标准）：
#   创建 / 列表（分页 + 状态筛选）/ 详情 / 更新 / 删除 /
#   字段校验 / 未认证 401 / 跨用户越权 403 / 不存在 404
# ============================================================
set -uo pipefail

BASE="${BASE:-http://localhost:8080/api/v1}"
BODY_FILE="$(mktemp)"
trap 'rm -f "$BODY_FILE"' EXIT

PASS=0
FAIL=0

ok()  { printf '  \033[32m✅ %s\033[0m\n' "$1"; PASS=$((PASS + 1)); }
bad() { printf '  \033[31m❌ %s\033[0m\n' "$1"; FAIL=$((FAIL + 1)); }
warn() { printf '  \033[33m⚠ %s\033[0m\n' "$1"; }

# expect <描述> <期望> <实际>
expect() {
  if [ "$2" = "$3" ]; then ok "$1（$3）"; else bad "$1 → 期望 $2，实际 $3"; fi
}

# request <METHOD> <PATH> [JSON] [TOKEN]  → 输出 HTTP 状态码
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

# 注册并登录指定前缀的用户，回显 access_token
register_and_login() {
  local name="$1"
  local username="m2_${name}_${SUFFIX}"
  local email="${username}@example.com"
  request POST /auth/register "$(json --arg u "$username" --arg e "$email" --arg p "$PASSWORD" \
    '{username:$u,email:$e,password:$p}')" >/dev/null
  request POST /auth/login "$(json --arg e "$email" --arg p "$PASSWORD" '{email:$e,password:$p}')" >/dev/null
  field '.data.access_token'
}

printf '\n== M2 验收开始 | 目标: %s ==\n\n' "$BASE"

if ! curl -s -o /dev/null --max-time 5 "${BASE}/health"; then
  warn "无法访问 ${BASE}/health，请确认后端已启动"
fi

# ---------- 0. 准备两个用户（用于越权测试） ----------
echo "[0] 注册并登录用户 A / B"
TOKEN_A="$(register_and_login a)"
TOKEN_B="$(register_and_login b)"
if [ -n "$TOKEN_A" ] && [ "$TOKEN_A" != "null" ]; then ok "用户 A 已登录"; else bad "用户 A 登录失败"; fi
if [ -n "$TOKEN_B" ] && [ "$TOKEN_B" != "null" ]; then ok "用户 B 已登录"; else bad "用户 B 登录失败"; fi

# ---------- 1. 创建 ----------
echo "[1] 创建绘本"
code=$(request POST /books "$(json --arg t '小红帽的故事' \
  '{title:$t,description:"一个勇敢小女孩的故事",status:"draft"}')" "$TOKEN_A")
expect "创建成功" 201 "$code"
BOOK_ID="$(field '.data.id')"
if [ -n "$BOOK_ID" ] && [ "$BOOK_ID" != "null" ]; then ok "返回绘本 ID"; else bad "未返回绘本 ID"; fi
expect "创建默认状态为 draft" "draft" "$(field '.data.status')"

echo "[2] 创建：标题为空被拒"
code=$(request POST /books "$(json '{title:"",description:"x"}')" "$TOKEN_A")
expect "空标题被拒" 422 "$code"

echo "[3] 创建：非法状态被拒"
code=$(request POST /books "$(json --arg t 'T' '{title:$t,status:"unknown"}')" "$TOKEN_A")
expect "非法状态被拒" 422 "$code"

# ---------- 2. 列表 / 分页 / 状态筛选 ----------
echo "[4] 创建多本用于分页与筛选"
request POST /books "$(json --arg t '森林冒险' '{title:$t,status:"published"}')" "$TOKEN_A" >/dev/null
request POST /books "$(json --arg t '星空旅行' '{title:$t,status:"published"}')" "$TOKEN_A" >/dev/null
request POST /books "$(json --arg t '海底世界' '{title:$t,status:"archived"}')" "$TOKEN_A" >/dev/null

echo "[5] 列表（默认分页）"
code=$(request GET /users/me/books "" "$TOKEN_A")
expect "列表成功" 200 "$code"
TOTAL="$(field '.data.total')"
if [ "$TOTAL" -ge 4 ] 2>/dev/null; then ok "total 统计正确（${TOTAL}）"; else bad "total 异常（${TOTAL}）"; fi

echo "[6] 列表分页 limit=1"
code=$(request GET "/users/me/books?page=1&limit=1" "" "$TOKEN_A")
expect "分页成功" 200 "$code"
expect "每页返回 1 条" 1 "$(field '.data.items | length')"
expect "分页回显 limit" 1 "$(field '.data.limit')"

echo "[7] 状态筛选 status=published"
code=$(request GET "/users/me/books?status=published" "" "$TOKEN_A")
expect "筛选成功" 200 "$code"
PUB_TOTAL="$(field '.data.total')"
if [ "$PUB_TOTAL" -ge 2 ] 2>/dev/null; then ok "published 数量正确（${PUB_TOTAL}）"; else bad "published 数量异常（${PUB_TOTAL}）"; fi
expect "结果仅含 published" "published" "$(field '[.data.items[].status] | unique | join(",")')"

echo "[8] 状态筛选 status=archived"
code=$(request GET "/users/me/books?status=archived" "" "$TOKEN_A")
expect "筛选成功" 200 "$code"
expect "结果仅含 archived" "archived" "$(field '[.data.items[].status] | unique | join(",")')"

# ---------- 3. 详情 ----------
echo "[9] 获取绘本详情"
code=$(request GET "/books/${BOOK_ID}" "" "$TOKEN_A")
expect "详情成功" 200 "$code"
expect "详情标题正确" "小红帽的故事" "$(field '.data.title')"

# ---------- 4. 跨用户越权（核心验收项） ----------
echo "[10] 用户 B 查看用户 A 的绘本"
code=$(request GET "/books/${BOOK_ID}" "" "$TOKEN_B")
expect "越权查看被拒" 403 "$code"

echo "[11] 用户 B 修改用户 A 的绘本"
code=$(request PUT "/books/${BOOK_ID}" "$(json --arg t '被篡改' '{title:$t}')" "$TOKEN_B")
expect "越权修改被拒" 403 "$code"

echo "[12] 用户 B 删除用户 A 的绘本"
code=$(request DELETE "/books/${BOOK_ID}" "" "$TOKEN_B")
expect "越权删除被拒" 403 "$code"

# ---------- 5. 更新 ----------
echo "[13] 用户 A 更新绘本"
code=$(request PUT "/books/${BOOK_ID}" "$(json --arg t '小红帽（修订）' \
  '{title:$t,description:"更新后的描述",status:"published"}')" "$TOKEN_A")
expect "更新成功" 200 "$code"
expect "标题已更新" "小红帽（修订）" "$(field '.data.title')"
expect "状态已更新" "published" "$(field '.data.status')"

echo "[14] 更新：标题为空被拒"
code=$(request PUT "/books/${BOOK_ID}" "$(json '{title:""}')" "$TOKEN_A")
expect "空标题被拒" 422 "$code"

# ---------- 6. 不存在与未认证 ----------
echo "[15] 不存在的绘本返回 404"
code=$(request GET "/books/00000000-0000-0000-0000-000000000000" "" "$TOKEN_A")
expect "不存在返回 404" 404 "$code"

echo "[16] 未携带 Token 访问"
code=$(request GET /users/me/books)
expect "未认证被拒" 401 "$code"

# ---------- 7. 删除 ----------
echo "[17] 用户 A 删除绘本"
code=$(request DELETE "/books/${BOOK_ID}" "" "$TOKEN_A")
expect "删除成功" 200 "$code"

echo "[18] 删除后再次获取"
code=$(request GET "/books/${BOOK_ID}" "" "$TOKEN_A")
expect "已删除返回 404" 404 "$code"

# ---------- 8. 用户隔离 ----------
echo "[19] 用户 B 的列表不包含用户 A 的绘本"
code=$(request GET /users/me/books "" "$TOKEN_B")
expect "列表成功" 200 "$code"
expect "用户 B 无绘本" 0 "$(field '.data.total')"

# ---------- 汇总 ----------
printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
