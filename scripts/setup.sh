#!/bin/bash

# StoryCanvas 项目初始化脚本

set -e

echo "🚀 开始初始化 StoryCanvas 项目..."

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 辅助函数
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# 检查依赖
echo "📦 检查依赖..."
command -v git >/dev/null 2>&1 || { print_error "需要安装 git"; exit 1; }
command -v cmake >/dev/null 2>&1 || { print_error "需要安装 cmake"; exit 1; }
command -v python3 >/dev/null 2>&1 || { print_error "需要安装 python3"; exit 1; }
command -v node >/dev/null 2>&1 || { print_error "需要安装 node"; exit 1; }
command -v docker >/dev/null 2>&1 || { print_warning "docker 未安装，将跳过 Docker 相关配置"; }

print_success "依赖检查完成"

# 初始化 Git
echo "🔧 初始化 Git..."
if [ ! -d .git ]; then
    git init
    print_success "Git 仓库已初始化"
else
    print_warning "Git 仓库已存在"
fi

# 创建 .env 文件
echo "📝 创建 .env 文件..."
if [ ! -f .env ]; then
    cp .env.example .env
    print_success ".env 文件已创建"
    print_warning "请编辑 .env 文件配置环境变量"
else
    print_warning ".env 文件已存在"
fi

# 安装后端依赖
echo "📦 安装后端依赖..."
cd backend
if [ ! -d build ]; then
    mkdir -p build
    cd build
    cmake ..
    print_success "CMake 配置完成"
    cd ..
else
    print_warning "build 目录已存在"
fi
cd ..

# 安装 AI Service 依赖
echo "📦 安装 AI Service 依赖..."
cd ai-service
if [ ! -d venv ]; then
    python3 -m venv venv
    source venv/bin/activate
    pip install --upgrade pip
    pip install -r requirements.txt
    print_success "AI Service 依赖安装完成"
    deactivate
else
    print_warning "虚拟环境已存在"
fi
cd ..

# 安装前端依赖
echo "📦 安装前端依赖..."
cd frontend
if [ ! -d node_modules ]; then
    npm install
    print_success "前端依赖安装完成"
else
    print_warning "node_modules 已存在"
fi
cd ..

# 创建必要的目录
echo "📁 创建必要的目录..."
mkdir -p storage/users
mkdir -p storage/temp
mkdir -p storage/exports
mkdir -p logs
print_success "目录创建完成"

# 创建 .gitkeep 文件
touch storage/users/.gitkeep
touch storage/temp/.gitkeep
touch storage/exports/.gitkeep

echo ""
echo "=========================================="
print_success "项目初始化完成！"
echo "=========================================="
echo ""
echo "下一步："
echo "1. 编辑 .env 文件配置环境变量"
echo "2. 启动数据库服务：docker-compose up -d postgres redis"
echo "3. 初始化数据库：psql -h localhost -U storycanvas -d storycanvas -f sql/init.sql"
echo "4. 启动后端服务：cd backend/build && ./StoryCanvasBackend"
echo "5. 启动 AI Service：cd ai-service && source venv/bin/activate && uvicorn app.main:app --reload"
echo "6. 启动前端开发服务器：cd frontend && npm run dev"
echo ""
echo "或者使用 Docker Compose 启动所有服务：docker-compose up -d"
echo ""
