#!/bin/bash

# StoryCanvas 项目初始化脚本
# 用法：./scripts/setup.sh

set -e

echo "🚀 开始初始化 StoryCanvas 项目..."

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_success() { echo -e "${GREEN}✓ $1${NC}"; }
print_warning() { echo -e "${YELLOW}⚠ $1${NC}"; }
print_error() { echo -e "${RED}✗ $1${NC}"; }

# ==================== 依赖检查 ====================
echo "📦 检查依赖..."
command -v git >/dev/null 2>&1 || { print_error "需要安装 git"; exit 1; }
command -v cmake >/dev/null 2>&1 || { print_error "需要安装 cmake"; exit 1; }
command -v python3 >/dev/null 2>&1 || { print_error "需要安装 python3"; exit 1; }
command -v node >/dev/null 2>&1 || { print_error "需要安装 node"; exit 1; }
command -v docker >/dev/null 2>&1 || { print_warning "docker 未安装，将跳过 Docker 相关配置"; }
print_success "依赖检查完成"

# ==================== Git 初始化 ====================
echo "🔧 初始化 Git..."
if [ ! -d .git ]; then
    git init
    print_success "Git 仓库已初始化"
else
    print_warning "Git 仓库已存在"
fi

# ==================== 环境变量 ====================
echo "📝 创建 .env 文件..."
if [ ! -f .env ]; then
    cp .env.example .env
    print_success ".env 文件已创建"
    print_warning "请编辑 .env 填写 DB_PASSWORD / JWT_SECRET / AI_SERVICE_API_KEY（必填）"
else
    print_warning ".env 文件已存在"
fi

# AI Service 独立环境变量（可选，用于单独启动 AI 服务）
if [ ! -f ai-service/.env ]; then
    cp ai-service/.env.example ai-service/.env
    print_success "ai-service/.env 已创建"
fi

# ==================== 目录准备 ====================
echo "📁 创建必要的目录..."
mkdir -p storage/users storage/temp logs nginx/ssl
touch storage/users/.gitkeep storage/temp/.gitkeep
print_success "目录创建完成"

# ==================== 后端 ====================
echo "📦 配置后端构建..."
if [ ! -d backend/build ]; then
    mkdir -p backend/build
    (cd backend/build && cmake ..)
    print_success "CMake 配置完成（构建产物：backend/build/storycanvas_backend）"
else
    print_warning "backend/build 已存在"
fi

# ==================== AI Service ====================
echo "📦 安装 AI Service 依赖..."
if [ ! -d ai-service/venv ]; then
    python3 -m venv ai-service/venv
    # shellcheck disable=SC1091
    (source ai-service/venv/bin/activate \
        && pip install --upgrade pip \
        && pip install -r ai-service/requirements.txt)
    print_success "AI Service 依赖安装完成"
else
    print_warning "AI Service 虚拟环境已存在"
fi

# ==================== 前端 ====================
echo "📦 安装前端依赖..."
if [ ! -d frontend/node_modules ]; then
    (cd frontend && npm install)
    print_success "前端依赖安装完成"
else
    print_warning "frontend/node_modules 已存在"
fi

echo ""
echo "=========================================="
print_success "项目初始化完成！"
echo "=========================================="
echo ""
echo "下一步："
echo "1. 编辑 .env 填写必填密钥（DB_PASSWORD / JWT_SECRET / AI_SERVICE_API_KEY）"
echo "2. 启动数据库：docker-compose up -d postgres redis"
echo "3. 初始化数据库：psql -h localhost -U storycanvas -d storycanvas -f sql/init.sql"
echo "4. 启动后端：cd backend/build && ./storycanvas_backend        # http://localhost:8080/health"
echo "5. 启动 AI 服务：cd ai-service && source venv/bin/activate && uvicorn app.main:app --reload"
echo "6. 启动前端：cd frontend && npm run dev                        # http://localhost:5173"
echo ""
echo "或者一键启动全部服务：docker-compose up -d"
echo ""
