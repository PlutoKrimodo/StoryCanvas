# WSL 开发环境搭建方案

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

**版本**: 1.0  
**日期**: 2026-09-17  
**作者**: StoryCanvas 开发团队

---

## 1. WSL 环境概述

### 1.1 为什么选择 WSL

- 原生 Linux 环境
- 与 Windows 文件系统集成
- 支持 Docker Desktop
- VS Code Remote 开发支持

### 1.2 推荐配置

| 组件 | 推荐版本 | 说明 |
|------|---------|------|
| WSL | WSL 2 | 性能更好 |
| 发行版 | Ubuntu 22.04 LTS | 稳定、文档丰富 |
| 内存 | 8GB+ | 足够运行所有服务 |
| 磁盘 | 50GB+ | 足够存储项目和依赖 |

---

## 2. 发行版选择

### 2.1 推荐：Ubuntu 22.04 LTS

**优点**：
- 长期支持版本
- 社区活跃，文档丰富
- 软件包更新及时
- 与 Docker 兼容性好

**安装命令**（在 PowerShell 中执行）：
```powershell
wsl --install -d Ubuntu-22.04
```

### 2.2 备选：Ubuntu 20.04 LTS

如果遇到兼容性问题，可以使用 20.04 版本。

---

## 3. 基础环境安装

### 3.1 系统更新

```bash
sudo apt update && sudo apt upgrade -y
```

### 3.2 基础工具

```bash
sudo apt install -y \
    build-essential \
    curl \
    wget \
    git \
    vim \
    htop \
    tree \
    unzip \
    software-properties-common \
    apt-transport-https \
    ca-certificates \
    gnupg \
    lsb-release
```

---

## 4. C++ 开发环境

### 4.1 编译器

```bash
# GCC
sudo apt install -y gcc g++

# 或者 Clang
sudo apt install -y clang clang++

# 验证版本
gcc --version
g++ --version
```

### 4.2 CMake

```bash
# 安装 CMake 3.20+
sudo apt install -y cmake

# 验证版本
cmake --version
```

### 4.3 Ninja（可选，构建加速）

```bash
sudo apt install -y ninja-build
```

### 4.4 Drogon 依赖

```bash
# 安装 Drogon 依赖
sudo apt install -y \
    libjsoncpp-dev \
    uuid-dev \
    openssl \
    libssl-dev \
    zlib1g-dev \
    libsqlite3-dev \
    postgresql-client \
    libpq-dev \
    redis-tools
```

### 4.5 安装 Drogon

```bash
# 克隆 Drogon
cd /tmp
git clone https://github.com/drogonframework/drogon.git
cd drogon

# 编译安装
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install

# 验证安装
drogon_ctl -v
```

---

## 5. Python 开发环境

### 5.1 安装 Python 3.10+

```bash
sudo apt install -y python3 python3-pip python3-venv

# 验证版本
python3 --version
pip3 --version
```

### 5.2 创建虚拟环境

```bash
# 进入项目目录
cd ~/StoryCanvas/ai-service

# 创建虚拟环境
python3 -m venv venv

# 激活虚拟环境
source venv/bin/activate

# 升级 pip
pip install --upgrade pip
```

### 5.3 安装依赖

```bash
# 安装项目依赖
pip install -r requirements.txt

# 安装开发依赖（可选）
pip install -r requirements-dev.txt
```

---

## 6. Node.js 开发环境

### 6.1 安装 Node.js

```bash
# 使用 NodeSource 安装 Node.js 18
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# 验证版本
node --version
npm --version
```

### 6.2 安装包管理器

```bash
# 安装 pnpm（推荐）
npm install -g pnpm

# 或者使用 yarn
npm install -g yarn
```

### 6.3 安装前端依赖

```bash
cd ~/StoryCanvas/frontend

# 使用 npm
npm install

# 或者使用 pnpm
pnpm install
```

---

## 7. 数据库环境

### 7.1 PostgreSQL

#### 方式一：直接安装

```bash
# 安装 PostgreSQL
sudo apt install -y postgresql postgresql-contrib

# 启动服务
sudo systemctl start postgresql
sudo systemctl enable postgresql

# 创建数据库和用户
sudo -u postgres psql
```

```sql
-- 创建用户
CREATE USER storycanvas WITH PASSWORD 'your_password';

-- 创建数据库
CREATE DATABASE storycanvas OWNER storycanvas;

-- 授予权限
GRANT ALL PRIVILEGES ON DATABASE storycanvas TO storycanvas;

-- 退出
\q
```

#### 方式二：使用 Docker（推荐）

```bash
# 拉取镜像
docker pull postgres:15

# 运行容器
docker run -d \
    --name storycanvas-postgres \
    -e POSTGRES_USER=storycanvas \
    -e POSTGRES_PASSWORD=your_password \
    -e POSTGRES_DB=storycanvas \
    -p 5432:5432 \
    -v postgres_data:/var/lib/postgresql/data \
    postgres:15
```

### 7.2 Redis

#### 方式一：直接安装

```bash
# 安装 Redis
sudo apt install -y redis-server

# 启动服务
sudo systemctl start redis-server
sudo systemctl enable redis-server

# 验证连接
redis-cli ping
```

#### 方式二：使用 Docker（推荐）

```bash
# 拉取镜像
docker pull redis:7-alpine

# 运行容器
docker run -d \
    --name storycanvas-redis \
    -p 6379:6379 \
    -v redis_data:/data \
    redis:7-alpine redis-server --appendonly yes
```

---

## 8. Docker 环境

### 8.1 安装 Docker Desktop

1. 下载 Docker Desktop for Windows
2. 安装并启动
3. 在设置中启用 WSL 2 集成

### 8.2 验证 Docker

```bash
# 验证 Docker
docker --version
docker-compose --version

# 运行测试容器
docker run hello-world
```

### 8.3 Docker Compose

```bash
# 安装 Docker Compose（如果未包含在 Docker Desktop 中）
sudo apt install -y docker-compose
```

---

## 9. VS Code 配置

### 9.1 安装 VS Code

1. 下载 Visual Studio Code
2. 安装并启动

### 9.2 安装 Remote Development 扩展

1. 打开 VS Code
2. 安装扩展：`Remote - WSL`
3. 安装扩展：`Remote - Containers`

### 9.3 在 WSL 中打开 VS Code

```bash
# 在项目目录中执行
cd ~/StoryCanvas
code .
```

### 9.4 推荐扩展

| 扩展 | 说明 |
|------|------|
| C/C++ | C++ 支持 |
| CMake Tools | CMake 支持 |
| Python | Python 支持 |
| Pylance | Python 语言支持 |
| ESLint | JavaScript 代码检查 |
| Prettier | 代码格式化 |
| Docker | Docker 支持 |
| GitLens | Git 增强 |

---

## 10. 项目代码管理

### 10.1 代码存放位置

**推荐**：将代码放在 WSL 文件系统中

```bash
# WSL 文件系统
~/StoryCanvas

# 不推荐：Windows 文件系统
/mnt/c/Users/xxx/StoryCanvas
```

**原因**：
- WSL 文件系统性能更好
- 避免文件权限问题
- Docker 挂载更稳定

### 10.2 Git 配置

```bash
# 配置用户信息
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 配置换行符处理
git config --global core.autocrlf input
```

### 10.3 SSH Key 配置

```bash
# 生成 SSH Key
ssh-keygen -t ed25519 -C "your.email@example.com"

# 启动 ssh-agent
eval "$(ssh-agent -s)"

# 添加 SSH Key
ssh-add ~/.ssh/id_ed25519

# 复制公钥
cat ~/.ssh/id_ed25519.pub
```

---

## 11. 环境变量管理

### 11.1 使用 .env 文件

```bash
# 创建 .env 文件
cd ~/StoryCanvas
cp .env.example .env

# 编辑 .env 文件
vim .env
```

### 11.2 .env 文件内容

```bash
# 数据库配置
DB_HOST=localhost
DB_PORT=5432
DB_NAME=storycanvas
DB_USER=storycanvas
DB_PASSWORD=your_password

# Redis 配置
REDIS_HOST=localhost
REDIS_PORT=6379

# JWT 配置
JWT_SECRET=your-jwt-secret

# AI API 配置
OPENAI_API_KEY=sk-your-openai-key
STABILITY_API_KEY=sk-your-stability-key

# AI Service 配置
AI_SERVICE_URL=http://localhost:8000
```

### 11.3 避免密钥泄露

```bash
# 确保 .env 在 .gitignore 中
echo ".env" >> .gitignore

# 不要提交敏感信息
git status  # 检查是否有 .env 文件
```

---

## 12. 常用命令

### 12.1 服务管理

```bash
# 启动 PostgreSQL
sudo systemctl start postgresql

# 启动 Redis
sudo systemctl start redis-server

# 查看服务状态
sudo systemctl status postgresql
sudo systemctl status redis-server
```

### 12.2 Docker 命令

```bash
# 启动所有服务
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down

# 重新构建
docker-compose build --no-cache
```

### 12.3 项目命令

```bash
# 编译 C++ 后端
cd ~/StoryCanvas/backend
mkdir build && cd build
cmake ..
make -j$(nproc)

# 启动 AI Service
cd ~/StoryCanvas/ai-service
source venv/bin/activate
uvicorn app.main:app --reload --port 8000

# 启动前端
cd ~/StoryCanvas/frontend
npm run dev
```

---

## 13. 故障排除

### 13.1 常见问题

#### 问题：Drogon 编译失败

```bash
# 检查依赖是否安装
sudo apt install -y libjsoncpp-dev uuid-dev libssl-dev

# 重新编译
cd /tmp/drogon/build
cmake ..
make -j$(nproc)
```

#### 问题：数据库连接失败

```bash
# 检查 PostgreSQL 服务
sudo systemctl status postgresql

# 检查连接
psql -h localhost -U storycanvas -d storycanvas
```

#### 问题：端口被占用

```bash
# 查找占用端口的进程
sudo lsof -i :8080

# 杀死进程
sudo kill -9 <PID>
```

### 13.2 获取帮助

- Drogon 文档：https://drogonframework.com/
- FastAPI 文档：https://fastapi.tiangolo.com/
- React 文档：https://react.dev/

---

## 14. 环境验证清单

```
□ C++ 环境
  □ gcc/g++ 已安装
  □ cmake 已安装
  □ drogon 已安装

□ Python 环境
  □ python3 已安装
  □ venv 已创建
  □ 依赖已安装

□ Node.js 环境
  □ node 已安装
  □ npm 已安装
  □ 前端依赖已安装

□ 数据库环境
  □ PostgreSQL 运行中
  □ Redis 运行中
  □ 数据库可连接

□ Docker 环境
  □ Docker Desktop 运行中
  □ docker-compose 可用

□ VS Code 环境
  □ Remote WSL 扩展已安装
  □ 项目可正常打开

□ 项目验证
  □ C++ 后端可编译
  □ Python AI Service 可启动
  □ 前端可启动
  □ 所有服务可正常通信
```

---

**文档结束**
