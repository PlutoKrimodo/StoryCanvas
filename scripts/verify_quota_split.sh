#!/usr/bin/env bash
# ============================================================
# 额度拆分验证：游客池 / 登录用户池 / 双项 BYOK 互不影响
#
# 前置：
#   - 已执行 sql/migrate_quota_split.sql
#   - 数据库里有至少一个游客用户和一个登录用户
#   - 本机可用 psql（PostgreSQL 客户端）
#
# 用法：
#   bash scripts/verify_quota_split.sh
# ============================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$ROOT_DIR/backend/.env"
[ -f "$ENV_FILE" ] || ENV_FILE="$ROOT_DIR/.env"

read_env() {
  local key="$1" fallback="$2"
  local value
  value="$(grep -E "^${key}=" "$ENV_FILE" 2>/dev/null | tail -1 | cut -d= -f2- | tr -d '"' || true)"
  printf '%s' "${value:-$fallback}"
}

DB_HOST="$(read_env DB_HOST localhost)"
DB_PORT="$(read_env DB_PORT 5432)"
DB_USER="$(read_env DB_USER storycanvas)"
DB_NAME="$(read_env DB_NAME storycanvas)"
DB_PASSWORD="$(read_env DB_PASSWORD "")"
export PGPASSWORD="$DB_PASSWORD"

psql_cmd() {
  psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -t -A "$@"
}

if ! psql_cmd -c "SELECT is_guest FROM generation_tasks LIMIT 1" >/dev/null 2>&1; then
  echo "FAIL: generation_tasks.is_guest 不存在，请先执行 sql/migrate_quota_split.sql" >&2
  exit 1
fi

GUEST_ID="$(psql_cmd -c "SELECT id::text FROM users WHERE is_guest = true LIMIT 1")"
USER_ID="$(psql_cmd -c "SELECT id::text FROM users WHERE is_guest = false LIMIT 1")"
if [ -z "$GUEST_ID" ] || [ -z "$USER_ID" ]; then
  echo "FAIL: 数据库缺少游客或登录用户，无法验证（请先注册/创建游客）" >&2
  exit 1
fi

MARKER="__quota_split_test__"

count_scope() {
  local guest="$1"
  psql_cmd -c "SELECT COUNT(*)::int FROM generation_tasks
    WHERE created_at >= (
      date_trunc('day', now() AT TIME ZONE 'Asia/Shanghai') AT TIME ZONE 'Asia/Shanghai')
      AND is_guest = $guest
      AND NOT (byok_llm AND byok_image)" | tr -d '[:space:]'
}

cleanup() {
  psql_cmd -c "DELETE FROM generation_tasks WHERE prompt = '$MARKER'" >/dev/null 2>&1 || true
}
trap cleanup EXIT

cleanup
GUEST_BEFORE="$(count_scope true)"
USER_BEFORE="$(count_scope false)"

# 1) 插入一个游客任务：游客池 +1，登录池不变
psql_cmd -c "INSERT INTO generation_tasks
  (user_id, prompt, status, byok_llm, byok_image, is_guest)
  VALUES ('$GUEST_ID', '$MARKER', 'completed', false, false, true)" >/dev/null

GUEST_AFTER="$(count_scope true)"
USER_AFTER="$(count_scope false)"
if [ "$GUEST_AFTER" -ne $((GUEST_BEFORE + 1)) ] || [ "$USER_AFTER" -ne "$USER_BEFORE" ]; then
  echo "FAIL: 游客任务影响了登录池（guest $GUEST_BEFORE -> $GUEST_AFTER, user $USER_BEFORE -> $USER_AFTER）" >&2
  exit 1
fi

# 2) 插入一个登录用户任务：登录池 +1，游客池不变
psql_cmd -c "INSERT INTO generation_tasks
  (user_id, prompt, status, byok_llm, byok_image, is_guest)
  VALUES ('$USER_ID', '$MARKER', 'completed', false, false, false)" >/dev/null

GUEST_AFTER2="$(count_scope true)"
USER_AFTER2="$(count_scope false)"
if [ "$USER_AFTER2" -ne $((USER_AFTER + 1)) ] || [ "$GUEST_AFTER2" -ne "$GUEST_AFTER" ]; then
  echo "FAIL: 登录任务影响了游客池（guest $GUEST_AFTER -> $GUEST_AFTER2, user $USER_AFTER -> $USER_AFTER2）" >&2
  exit 1
fi

# 3) 插入双项 BYOK 任务：两个池都不增加
psql_cmd -c "INSERT INTO generation_tasks
  (user_id, prompt, status, byok_llm, byok_image, is_guest)
  VALUES ('$USER_ID', '$MARKER', 'completed', true, true, false)" >/dev/null

GUEST_AFTER3="$(count_scope true)"
USER_AFTER3="$(count_scope false)"
if [ "$GUEST_AFTER3" -ne "$GUEST_AFTER2" ] || [ "$USER_AFTER3" -ne "$USER_AFTER2" ]; then
  echo "FAIL: 双项 BYOK 任务不应计入任何池（guest $GUEST_AFTER2 -> $GUEST_AFTER3, user $USER_AFTER2 -> $USER_AFTER3）" >&2
  exit 1
fi

# 4) 清理后计数复原
cleanup
GUEST_FINAL="$(count_scope true)"
USER_FINAL="$(count_scope false)"
if [ "$GUEST_FINAL" -ne "$GUEST_BEFORE" ] || [ "$USER_FINAL" -ne "$USER_BEFORE" ]; then
  echo "FAIL: 清理后计数未复原（guest $GUEST_BEFORE -> $GUEST_FINAL, user $USER_BEFORE -> $USER_FINAL）" >&2
  exit 1
fi

echo "PASS: 游客池 +1 不影响登录池；登录池 +1 不影响游客池；双项 BYOK 不计入；清理后复原。"
