#!/usr/bin/env bash
# ============================================================
# Phase 2（M1 用户系统）验收脚本
#
# 前置条件：
#   1. PostgreSQL 已初始化（psql -f sql/init.sql）
#   2. 后端已启动，且 JWT_SECRET / DB_PASSWORD 等已通过环境变量注入
#   3. 已安装 curl 与 jq
#
# 用法：
#   ./scripts/verify_m1.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_m1.sh
#   DB_DSN='postgresql://storycanvas:密码@localhost:5432/storycanvas' ./scripts/verify_m1.sh
#
# 覆盖验收项（对应 docs/16 第 8.2 节 M1 验收标准）：
#   注册 / 登录 / 携 Token 访问用户信息 / 无 Token 401 / 篡改 Token 401 /
#   刷新 Token / 账号枚举防护 / 改密后旧 Token 失效 / 登出后 Token 失效 /
#   密码为单向哈希入库
# ============================================================
set -uo pipefail

BASE="${BASE:-http://localhost:8080/api/v1}"
DB_DSN="${DB_DSN:-}"
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
USERNAME="m1_${SUFFIX}"
EMAIL="${USERNAME}@example.com"
PASSWORD='StrongPass1!'
NEW_PASSWORD='StrongPass2!'

printf '\n== M1 验收开始 | 目标: %s ==\n\n' "$BASE"

if ! curl -s -o /dev/null --max-time 5 "${BASE}/health"; then
  warn "无法访问 ${BASE}/health，请确认后端已启动"
fi

# ---------- 1. 注册 ----------
echo "[1] 注册新用户"
code=$(request POST /auth/register "$(json --arg u "$USERNAME" --arg e "$EMAIL" --arg p "$PASSWORD" \
  '{username:$u,email:$e,password:$p}')")
expect "注册成功" 201 "$code"

echo "[2] 重复用户名注册"
code=$(request POST /auth/register "$(json --arg u "$USERNAME" --arg e "dup_${EMAIL}" --arg p "$PASSWORD" \
  '{username:$u,email:$e,password:$p}')")
expect "用户名冲突" 409 "$code"

echo "[3] 弱密码注册"
code=$(request POST /auth/register "$(json --arg u "weak_${SUFFIX}" --arg e "weak_${EMAIL}" \
  '{username:$u,email:$e,password:"123"}')")
expect "弱密码被拒" 422 "$code"

# ---------- 2. 登录 ----------
echo "[4] 正确密码登录"
code=$(request POST /auth/login "$(json --arg e "$EMAIL" --arg p "$PASSWORD" '{email:$e,password:$p}')")
expect "登录成功" 200 "$code"
ACCESS="$(field '.data.access_token')"
REFRESH="$(field '.data.refresh_token')"
if [ -n "$ACCESS" ] && [ "$ACCESS" != "null" ]; then ok "返回 access_token"; else bad "未返回 access_token"; fi

echo "[5] 账号枚举防护（错误密码 vs 不存在邮箱）"
code=$(request POST /auth/login "$(json --arg e "$EMAIL" '{email:$e,password:"WrongPass1!"}')")
expect "错误密码" 401 "$code"
MSG_WRONG="$(field '.message')"

code=$(request POST /auth/login "$(json --arg e "nobody_${SUFFIX}@example.com" '{email:$e,password:"WrongPass1!"}')")
expect "不存在邮箱" 401 "$code"
MSG_UNKNOWN="$(field '.message')"
expect "两者错误文案一致" "$MSG_WRONG" "$MSG_UNKNOWN"

# ---------- 3. 鉴权 ----------
echo "[6] 携带 Token 访问 /users/me"
code=$(request GET /users/me "" "$ACCESS")
expect "鉴权通过" 200 "$code"
if [ "$(field '.data.password')" = "null" ] && [ "$(field '.data.password_hash')" = "null" ]; then
  ok "响应不含密码字段"
else
  bad "响应泄露了密码相关字段"
fi

echo "[7] 无 Token 访问"
code=$(request GET /users/me)
expect "未认证被拒" 401 "$code"

echo "[8] 篡改 Token 访问"
code=$(request GET /users/me "" "${ACCESS}x")
expect "签名校验失败" 401 "$code"

# ---------- 4. 刷新 ----------
echo "[9] 刷新 Token"
code=$(request POST /auth/refresh "$(json --arg t "$REFRESH" '{refresh_token:$t}')")
expect "刷新成功" 200 "$code"
ACCESS2="$(field '.data.access_token')"
REFRESH2="$(field '.data.refresh_token')"
if [ -n "$ACCESS2" ] && [ "$ACCESS2" != "null" ]; then ok "换发新 access_token"; else bad "未换发新 access_token"; fi

# ---------- 5. 修改密码 ----------
echo "[10] 修改密码：原密码错误"
code=$(request PUT /users/me/password "$(json --arg o 'WrongOld1' --arg n "$NEW_PASSWORD" \
  '{old_password:$o,new_password:$n}')" "$ACCESS2")
expect "原密码校验失败" 400 "$code"

echo "[11] 修改密码：正确原密码"
code=$(request PUT /users/me/password "$(json --arg o "$PASSWORD" --arg n "$NEW_PASSWORD" \
  '{old_password:$o,new_password:$n}')" "$ACCESS2")
expect "修改成功" 200 "$code"

echo "[12] 改密后旧 Token 失效（Phase 2 核心安全项）"
code=$(request GET /users/me "" "$ACCESS2")
expect "旧 access_token 已撤销" 401 "$code"
code=$(request POST /auth/refresh "$(json --arg t "$REFRESH2" '{refresh_token:$t}')")
expect "旧 refresh_token 已撤销" 401 "$code"

# ---------- 6. 新密码登录与登出 ----------
echo "[13] 使用新密码登录"
code=$(request POST /auth/login "$(json --arg e "$EMAIL" --arg p "$NEW_PASSWORD" '{email:$e,password:$p}')")
expect "新密码可登录" 200 "$code"
ACCESS3="$(field '.data.access_token')"

echo "[14] 登出后 Token 失效"
code=$(request POST /auth/logout "" "$ACCESS3")
expect "登出成功" 200 "$code"
code=$(request GET /users/me "" "$ACCESS3")
expect "已登出 Token 被拒" 401 "$code"

# ---------- 7. 数据库密码字段 ----------
echo "[15] 数据库密码字段为单向哈希"
if command -v psql >/dev/null && [ -n "$DB_DSN" ]; then
  HASH="$(psql "$DB_DSN" -tAc "SELECT password_hash FROM users WHERE email='${EMAIL}'" 2>/dev/null)"
  case "$HASH" in
    \$2a\$*|\$2b\$*|\$2y\$*|pbkdf2_sha256\$*) ok "password_hash 为单向哈希（${HASH:0:7}…）" ;;
    "") bad "未查询到 password_hash，请检查 DB_DSN" ;;
    *)  bad "password_hash 格式异常：${HASH:0:12}…" ;;
  esac
else
  warn "跳过（需安装 psql 并设置 DB_DSN 环境变量）"
fi

# ---------- 汇总 ----------
printf '\n== 结果：通过 %d，失败 %d ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
