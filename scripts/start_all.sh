#!/usr/bin/env bash
# ============================================================
# StoryCanvas 一键启动全部服务（本地开发 / 浏览器测试）
#
# 顺序：PostgreSQL + Redis（docker compose）→ C++ 后端 → Python AI Service → React 前端
# 启动后保持前台运行；按 Ctrl+C 会一次性停掉本次启动的所有服务。
#
# 用法：
#   ./scripts/start_all.sh                 启动全部，保持运行
#   ./scripts/start_all.sh --no-ai         不起 AI Service
#   ./scripts/start_all.sh --no-frontend   不起前端
#   ./scripts/start_all.sh --rebuild       强制重新构建后端
#   ./scripts/start_all.sh --init-db       用 psql 执行 sql/init.sql（本机 PostgreSQL 场景）
#   ./scripts/start_all.sh --verify        启动后再跑一遍 verify_m1 / verify_m2
#   ./scripts/start_all.sh --open          尝试自动打开浏览器
#   ./scripts/start_all.sh --down          停止 compose 服务后退出
#
# 日志：logs/start_all/*.log    PID：logs/start_all/*.pid
#      （logs/ 已被 .gitignore 忽略）
# ============================================================
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="$ROOT_DIR/logs/start_all"

WITH_FRONTEND=1
WITH_AI=1
REBUILD=0
INIT_DB=0
VERIFY=0
OPEN_BROWSER=0
DO_DOWN=0

PIDS=()
CLEANUP_DONE=0

# ---------- 颜色与输出 ----------
if [ -t 1 ]; then
  C_RED=$'\033[31m'; C_GREEN=$'\033[32m'; C_YELLOW=$'\033[33m'
  C_CYAN=$'\033[36m'; C_BOLD=$'\033[1m'; C_DIM=$'\033[2m'; C_NC=$'\033[0m'
else
  C_RED=''; C_GREEN=''; C_YELLOW=''; C_CYAN=''; C_BOLD=''; C_DIM=''; C_NC=''
fi

info()  { printf '%s\n' "${C_DIM}· $*${C_NC}"; }
ok()    { printf '%s\n' "${C_GREEN}✅ $*${C_NC}"; }
warn()  { printf '%s\n' "${C_YELLOW}⚠ $*${C_NC}"; }
err()   { printf '%s\n' "${C_RED}✗ $*${C_NC}" >&2; }
step()  { printf '\n%s\n' "${C_CYAN}▶ $*${C_NC}"; }

usage() {
  cat <<'EOF'
用法：./scripts/start_all.sh [选项]

启动 PostgreSQL/Redis → 后端 → AI Service → 前端，并在前台保持运行。

选项：
  --no-ai           不启动 AI Service
  --no-frontend     不启动前端
  --rebuild         强制重新构建后端
  --init-db         用 psql 执行 sql/init.sql（本机 PostgreSQL 场景）
  --verify          启动后额外运行 verify_m1.sh 与 verify_m2.sh
  --open            尝试自动打开浏览器
  --down            停止 docker compose 服务后退出
  -h, --help        显示本帮助

停止：在终端按 Ctrl+C（会停掉本次启动的全部服务）
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --no-ai)        WITH_AI=0 ;;
    --no-frontend)  WITH_FRONTEND=0 ;;
    --rebuild)      REBUILD=1 ;;
    --init-db)      INIT_DB=1 ;;
    --verify)       VERIFY=1 ;;
    --open)         OPEN_BROWSER=1 ;;
    --down)         DO_DOWN=1 ;;
    -h|--help)      usage; exit 0 ;;
    *) err "未知参数：$1"; usage; exit 2 ;;
  esac
  shift
done

# ---------- 基础工具函数 ----------
require() {
  command -v "$1" >/dev/null 2>&1 || { err "缺少命令：$1，请先安装"; exit 3; }
}

# port_open <host> <port>
port_open() { (exec 3<>"/dev/tcp/$1/$2") 2>/dev/null; }

# env_get <KEY> [默认值]：从根目录 .env 读取
env_get() {
  local key="$1" default="${2:-}" val
  [ -f "$ROOT_DIR/.env" ] || { printf '%s' "$default"; return; }
  val="$(sed -n "s/^[[:space:]]*${key}=//p" "$ROOT_DIR/.env" | tail -n1)"
  val="${val%\"}"; val="${val#\"}"
  printf '%s' "${val:-$default}"
}

check_env_key() {
  local key="$1" desc="$2" val
  val="$(env_get "$key")"
  if [ -z "$val" ]; then
    warn "${key} 未配置（${desc}）"
    return 1
  fi
  case "$val" in
    change_me*|your_*|sk-your_*)
      warn "${key} 仍是示例值，请改成真实值（${desc}）"
      return 1
      ;;
  esac
  return 0
}

wait_for_port() {
  local host="$1" port="$2" timeout="${3:-30}" name="${4:-服务}" start=$SECONDS
  while [ $((SECONDS - start)) -lt "$timeout" ]; do
    if port_open "$host" "$port"; then return 0; fi
    sleep 1
  done
  warn "${name} 在 ${timeout}s 内未就绪（${host}:${port}）"
  return 1
}

wait_for_http() {
  local url="$1" timeout="${2:-30}" name="${3:-服务}" start=$SECONDS
  while [ $((SECONDS - start)) -lt "$timeout" ]; do
    if curl -fsS -o /dev/null --max-time 2 "$url" 2>/dev/null; then return 0; fi
    sleep 1
  done
  warn "${name} 在 ${timeout}s 内未通过健康检查（${url}）"
  return 1
}

# start_bg <名称> <工作目录> <命令...>
start_bg() {
  local name="$1" workdir="$2"; shift 2
  local logfile="$LOG_DIR/${name}.log"
  ( cd "$workdir" && exec "$@" ) >"$logfile" 2>&1 &
  local pid=$!
  PIDS+=("$pid")
  printf '%s\n' "$pid" >"$LOG_DIR/${name}.pid"
  info "已启动 ${name}（pid=${pid}，日志 logs/start_all/${name}.log）"
}

dump_log() {
  local name="$1" lines="${2:-25}" logfile="$LOG_DIR/${name}.log"
  if [ -f "$logfile" ]; then
    warn "—— ${name} 日志末尾（${lines} 行）——"
    tail -n "$lines" "$logfile" | sed 's/^/    /'
  fi
}

# 递归终止进程树：npm run dev 会派生子进程，单杀父进程会残留
kill_tree() {
  local pid="$1" child
  if command -v pgrep >/dev/null 2>&1; then
    for child in $(pgrep -P "$pid" 2>/dev/null); do
      kill_tree "$child"
    done
  fi
  kill "$pid" 2>/dev/null || true
}

cleanup() {
  [ "$CLEANUP_DONE" -eq 1 ] && return
  CLEANUP_DONE=1
  [ "${#PIDS[@]}" -eq 0 ] && return
  printf '\n'
  info "正在停止本次启动的服务..."
  local pid
  for pid in "${PIDS[@]}"; do
    kill_tree "$pid"
  done
  wait 2>/dev/null || true
  ok "已停止（数据库容器仍在后台，如需停止：./scripts/start_all.sh --down）"
}
trap cleanup EXIT INT TERM

ensure_node() {
  command -v node >/dev/null 2>&1 && return 0
  export NVM_DIR="${NVM_DIR:-$HOME/.nvm}"
  # shellcheck disable=SC1091
  [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh"
  command -v node >/dev/null 2>&1
}

open_browser() {
  local url="$1"
  if command -v wslview >/dev/null 2>&1; then
    ( wslview "$url" >/dev/null 2>&1 & )
  elif command -v xdg-open >/dev/null 2>&1; then
    ( xdg-open "$url" >/dev/null 2>&1 & )
  elif command -v open >/dev/null 2>&1; then
    ( open "$url" >/dev/null 2>&1 & )
  else
    warn "未找到可用的浏览器打开命令，请手动访问 ${url}"
  fi
}

# ---------- 依赖与配置检查 ----------
mkdir -p "$LOG_DIR"
cd "$ROOT_DIR" || exit 3

step "0. 检查依赖与配置"
require curl
[ -f "$ROOT_DIR/.env" ] || { err "未找到 .env（先执行 cp .env.example .env 并填写）"; exit 3; }

ENV_OK=1
check_env_key DB_PASSWORD "数据库密码" || ENV_OK=0
check_env_key JWT_SECRET "JWT 签名密钥，为空后端会直接退出" || ENV_OK=0
check_env_key AI_SERVICE_API_KEY "后端 ↔ AI 服务内网鉴权" || ENV_OK=0
if [ "$ENV_OK" -eq 0 ]; then
  err ".env 必填项未就绪，已中止。生成密钥：openssl rand -hex 32"
  exit 3
fi
ok ".env 检查通过"

DB_HOST_V="$(env_get DB_HOST localhost)"
DB_PORT_V="$(env_get DB_PORT 5432)"
DB_USER_V="$(env_get DB_USER storycanvas)"
DB_NAME_V="$(env_get DB_NAME storycanvas)"
BACKEND_PORT_V="$(env_get BACKEND_PORT 8080)"
AI_PORT_V="$(env_get AI_SERVICE_PORT 8000)"
FRONTEND_PORT_V="$(env_get FRONTEND_PORT 5173)"

DC=()
if docker compose version >/dev/null 2>&1; then
  DC=(docker compose)
elif command -v docker-compose >/dev/null 2>&1; then
  DC=(docker-compose)
fi

if [ "$DO_DOWN" -eq 1 ]; then
  step "停止 compose 服务"
  [ "${#DC[@]}" -gt 0 ] || { err "未找到 docker compose"; exit 3; }
  "${DC[@]}" down
  ok "已停止"
  trap - EXIT
  exit 0
fi

# ---------- 1. 数据库 ----------
step "1. 启动数据库"
if port_open "$DB_HOST_V" "$DB_PORT_V"; then
  info "数据库端口已在监听（${DB_HOST_V}:${DB_PORT_V}），跳过启动"
elif [ "${#DC[@]}" -gt 0 ]; then
  "${DC[@]}" up -d postgres redis || { err "docker compose 启动数据库失败"; exit 1; }
  wait_for_port "$DB_HOST_V" "$DB_PORT_V" 60 "PostgreSQL" || { err "数据库启动超时"; exit 1; }
  ok "数据库就绪"
else
  err "端口 ${DB_HOST_V}:${DB_PORT_V} 未监听，且未安装 docker compose"
  err "请先启动本机 PostgreSQL，或安装 Docker 后重试"
  exit 1
fi

if [ "$INIT_DB" -eq 1 ]; then
  step "1.1 初始化数据库（--init-db）"
  if command -v psql >/dev/null 2>&1; then
    PGPASSWORD="$(env_get DB_PASSWORD)" psql -h "$DB_HOST_V" -p "$DB_PORT_V" \
      -U "$DB_USER_V" -d "$DB_NAME_V" -f "$ROOT_DIR/sql/init.sql"
  else
    warn "未找到 psql，跳过（Docker 场景首次启动会自动执行 init.sql）"
  fi
fi

# ---------- 2. 后端 ----------
step "2. 构建并启动后端"
if port_open localhost "$BACKEND_PORT_V"; then
  warn "端口 ${BACKEND_PORT_V} 已被占用，假定后端已在运行，跳过启动"
else
  BIN="$ROOT_DIR/backend/build/storycanvas_backend"
  if [ "$REBUILD" -eq 1 ] || [ ! -x "$BIN" ]; then
    require cmake
    require make
    mkdir -p "$ROOT_DIR/backend/build"
    info "编译后端（首次或 --rebuild 时较慢）..."
    ( cd "$ROOT_DIR/backend/build" && cmake .. >/dev/null && make -j"$(nproc)" ) \
      || { err "后端编译失败"; exit 1; }
  else
    info "后端可执行文件已存在，跳过编译（--rebuild 可强制重建）"
  fi

  # ConfigManager 以「相对当前工作目录」读取 ./.env 与 ./config/config.json
  if [ -f "$ROOT_DIR/.env" ] && [ ! -f "$ROOT_DIR/backend/.env" ]; then
    cp "$ROOT_DIR/.env" "$ROOT_DIR/backend/.env"
    info "已复制根 .env → backend/.env（供后端相对路径读取）"
  fi

  start_bg backend "$ROOT_DIR/backend" ./build/storycanvas_backend
  if wait_for_http "http://localhost:${BACKEND_PORT_V}/health" 30 "后端"; then
    ok "后端就绪"
  else
    dump_log backend 30
    err "后端未启动成功，请查看上方日志"
    exit 1
  fi
fi

# ---------- 3. AI Service ----------
if [ "$WITH_AI" -eq 1 ]; then
  step "3. 启动 AI Service"
  if port_open localhost "$AI_PORT_V"; then
    info "端口 ${AI_PORT_V} 已在监听，跳过启动"
  elif [ ! -d "$ROOT_DIR/ai-service/venv" ]; then
    warn "未找到 ai-service/venv，跳过（可执行 scripts/setup.sh 创建）"
  else
    start_bg ai-service "$ROOT_DIR/ai-service" bash -c 'source venv/bin/activate && exec python main.py'
    if wait_for_http "http://localhost:${AI_PORT_V}/api/v1/health/" 30 "AI Service"; then
      ok "AI Service 就绪"
    else
      dump_log ai-service 20
      warn "AI Service 未就绪（用户/绘本功能不受影响，但无法生成插画）"
    fi
  fi
fi

# ---------- 4. 前端 ----------
if [ "$WITH_FRONTEND" -eq 1 ]; then
  step "4. 启动前端开发服务器"
  if port_open localhost "$FRONTEND_PORT_V"; then
    info "端口 ${FRONTEND_PORT_V} 已在监听，跳过启动"
  else
    ensure_node || { err "未找到 node，请先安装 Node.js 18+（或 nvm）"; exit 1; }
    if [ ! -d "$ROOT_DIR/frontend/node_modules" ]; then
      info "安装前端依赖（首次较慢）..."
      ( cd "$ROOT_DIR/frontend" && npm install --no-audit --no-fund ) \
        || { err "npm install 失败"; exit 1; }
    fi
    start_bg frontend "$ROOT_DIR/frontend" npm run dev
    if wait_for_http "http://localhost:${FRONTEND_PORT_V}/" 40 "前端"; then
      ok "前端就绪"
    else
      dump_log frontend 25
      err "前端未启动成功，请查看上方日志"
      exit 1
    fi
  fi
fi

# ---------- 5. 可选：自动化验收 ----------
if [ "$VERIFY" -eq 1 ]; then
  step "5. 运行接口验收（--verify）"
  if command -v jq >/dev/null 2>&1; then
    BASE="http://localhost:${BACKEND_PORT_V}/api/v1" bash "$ROOT_DIR/scripts/verify_m1.sh"
    BASE="http://localhost:${BACKEND_PORT_V}/api/v1" bash "$ROOT_DIR/scripts/verify_m2.sh"
  else
    warn "未安装 jq，跳过验收（sudo apt install jq）"
  fi
fi

# ---------- 就绪提示 ----------
SITE_URL="http://localhost:${FRONTEND_PORT_V}"
printf '\n%s\n' "${C_BOLD}════════════════ 服务已就绪 ════════════════${C_NC}"
printf '  %s浏览器打开：%s%s%s\n' "${C_BOLD}" "${C_GREEN}" "${SITE_URL}" "${C_NC}"
printf '  后端健康：  http://localhost:%s/health\n' "${BACKEND_PORT_V}"
printf '  AI 服务：   http://localhost:%s/api/v1/health/\n' "${AI_PORT_V}"
printf '  运行日志：  logs/start_all/*.log\n'
printf '%s\n' "${C_BOLD}════════════════════════════════════════════${C_NC}"
warn "按 Ctrl+C 停止全部服务"

if [ "$OPEN_BROWSER" -eq 1 ]; then
  open_browser "$SITE_URL"
fi

# 前台驻留，直到 Ctrl+C 或任一服务退出
wait
