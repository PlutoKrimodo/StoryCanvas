#!/usr/bin/env bash
# ============================================================
# BYOK（用户自带 AI API Key）验收脚本
#
# 前置条件：
#   1. 数据库已执行 sql/migrate_byok.sql（新库用 init.sql）
#   2. 后端已启动，BYOK_ENABLED=true 且已配置 ENCRYPTION_KEY
#   3. 已安装 curl 与 jq
#
# 覆盖验收项：
#   - 未登录 / 游客调用配置接口 → 401 / 403
#   - 厂商目录返回预设与申请链接
#   - 保存后仅回显脱敏 Key（sk-****abcd），响应全文不含明文
#   - 不重填 Key 可更新模型
#   - 删除后 overview 回到未配置
#   - BYOK_ENABLED=false 时接口 404（需重启为关闭状态后单独执行）
#
# 用法：
#   ./scripts/verify_byok.sh
#   BASE=http://localhost:8080/api/v1 ./scripts/verify_byok.sh
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
USERNAME="byok_${SUFFIX}"
EMAIL="${USERNAME}@example.com"
PASSWORD='StrongPass1!'
# 故意使用明显的假 Key；测试接口会失败，但「仍然保存」路径照常验证
FAKE_KEY="sk-byok-verify-${SUFFIX}-123456"

printf '\n== BYOK 验收开始 | 目标: %s ==\n\n' "$BASE"

# ---------- 0. 身份 ----------
echo "[0] 游客与登录用户"
EXPECTED_401="$(request GET /users/me/ai-credentials)"
expect "未登录访问配置接口返回 401" 401 "$EXPECTED_401"

request POST /auth/guest '{}' >/dev/null
GUEST_TOKEN="$(field '.data.guest_token')"
if [ -n "$GUEST_TOKEN" ] && [ "$GUEST_TOKEN" != "null" ]; then
  STATUS="$(request GET /users/me/ai-credentials '' "$GUEST_TOKEN")"
  expect "游客访问配置接口返回 403" 403 "$STATUS"
else
  warn "未能创建游客会话，跳过游客 403 校验"
fi

request POST /auth/register "$(json --arg u "$USERNAME" --arg e "$EMAIL" --arg p "$PASSWORD" \
  '{username:$u,email:$e,password:$p}')" >/dev/null
request POST /auth/login "$(json --arg e "$EMAIL" --arg p "$PASSWORD" '{email:$e,password:$p}')" >/dev/null
TOKEN="$(field '.data.access_token')"
if [ -n "$TOKEN" ] && [ "$TOKEN" != "null" ]; then ok "测试用户已登录"; else bad "测试用户登录失败"; exit 1; fi

# ---------- 1. 厂商目录 ----------
echo "[1] 厂商目录"
STATUS="$(request GET /users/me/ai-credentials/providers '' "$TOKEN")"
expect "目录接口返回 200" 200 "$STATUS"
if [ "$(field '.data.llm.providers | length')" -gt 0 ]; then
  ok "LLM 厂商目录非空"
else
  bad "LLM 厂商目录为空"
fi
if jq -e '.data.image.providers[] | select(.id == "doubao")' "$BODY_FILE" >/dev/null 2>&1; then
  ok "包含豆包图像预设"
else
  bad "缺少豆包图像预设"
fi

# ---------- 2. 保存与脱敏 ----------
echo "[2] 保存 LLM 凭据"
STATUS="$(request PUT /users/me/ai-credentials/llm \
  "$(json --arg k "$FAKE_KEY" '{provider:"deepseek",api_key:$k}')" "$TOKEN")"
expect "保存返回 200" 200 "$STATUS"
HINT="$(field '.data.api_key_hint')"
if [ "$HINT" != "null" ] && [ -n "$HINT" ] && [ "$HINT" != "$FAKE_KEY" ]; then
  ok "回显为脱敏值（$HINT）"
else
  bad "回显未脱敏或为空（$HINT）"
fi
if grep -q "$FAKE_KEY" "$BODY_FILE"; then
  bad "响应体包含明文 Key"
else
  ok "响应体不含明文 Key"
fi

STATUS="$(request GET /users/me/ai-credentials '' "$TOKEN")"
expect "overview 返回 200" 200 "$STATUS"
if jq -e '.data.credentials.llm != null' "$BODY_FILE" >/dev/null 2>&1; then
  ok "overview 含已保存的 LLM 凭据"
else
  bad "overview 缺少 LLM 凭据"
fi
if grep -q "$FAKE_KEY" "$BODY_FILE"; then
  bad "overview 包含明文 Key"
else
  ok "overview 不含明文 Key"
fi

# ---------- 3. 不重填 Key 更新模型 ----------
echo "[3] 更新模型（api_key 省略）"
STATUS="$(request PUT /users/me/ai-credentials/llm \
  '{"provider":"deepseek","model":"deepseek-reasoner"}' "$TOKEN")"
expect "更新返回 200" 200 "$STATUS"
MODEL="$(field '.data.model')"
expect "模型已更新" "deepseek-reasoner" "$MODEL"

# ---------- 4. 测试连接（假 Key 应可读失败，但接口 200） ----------
echo "[4] 测试连接"
STATUS="$(request POST /users/me/ai-credentials/test \
  "$(json --arg k "$FAKE_KEY" '{capability:"llm",provider:"deepseek",api_key:$k}')" "$TOKEN")"
expect "测试接口返回 200" 200 "$STATUS"
if [ "$(field '.data.ok')" = "false" ]; then
  ok "假 Key 返回可读失败（$(field '.data.message')）"
else
  warn "假 Key 未被拒绝（若配置了真实可用的 DeepSeek 网关请忽略）"
fi

# ---------- 5. 删除 ----------
echo "[5] 移除凭据"
STATUS="$(request DELETE /users/me/ai-credentials/llm '' "$TOKEN")"
expect "移除返回 200" 200 "$STATUS"
STATUS="$(request GET /users/me/ai-credentials '' "$TOKEN")"
expect "overview 返回 200" 200 "$STATUS"
if jq -e '.data.credentials.llm == null' "$BODY_FILE" >/dev/null 2>&1; then
  ok "凭据已移除"
else
  bad "凭据仍存在"
fi

printf '\n== 结果：%d 通过 / %d 失败 ==\n\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
