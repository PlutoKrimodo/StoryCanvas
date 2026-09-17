# StoryCanvas Phase 0 & Phase 1 开发记录

> 本文档以初学者视角，详细记录了项目从零开始搭建开发环境（Phase 0）和创建项目骨架（Phase 1）的完整过程。

---

## 目录

- [一、Phase 0：开发环境搭建](#一phase-0开发环境搭建)
  - [1.1 什么是开发环境？](#11-什么是开发环境)
  - [1.2 为什么需要这些工具？](#12-为什么需要这些工具)
  - [1.3 安装过程详解](#13-安装过程详解)
  - [1.4 验证环境](#14-验证环境)
- [二、Phase 1：项目骨架搭建](#二phase-1项目骨架搭建)
  - [2.1 什么是项目骨架？](#21-什么是项目骨架)
  - [2.2 C++ 后端（Backend）](#22-c-后端backend)
  - [2.3 Python AI Service](#23-python-ai-service)
  - [2.4 React 前端（Frontend）](#24-react-前端frontend)
  - [2.5 数据库初始化](#25-数据库初始化)
- [三、项目整体结构](#三项目整体结构)
- [四、常用启动命令速查](#四常用启动命令速查)
- [五、遇到的问题与解决方案](#五遇到的问题与解决方案)

---

## 一、Phase 0：开发环境搭建

### 1.1 什么是开发环境？

在写代码之前，我们需要在电脑上安装一系列**工具和软件**，这些统称为"开发环境"。就像厨师做菜前需要准备好锅碗瓢盆和食材一样，程序员写代码前也需要准备好各种工具。

### 1.2 为什么需要这些工具？

StoryCanvas 是一个**前后端分离**的项目，包含三个独立的子系统：

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  React 前端  │────▶│ C++ 后端    │────▶│ Python AI   │
│  (用户界面)  │     │ (业务逻辑)  │     │ (AI 生成)   │
└─────────────┘     └─────────────┘     └─────────────┘
                                            │
                                        ┌───▼───┐
                                        │PostgreSQL│
                                        │(数据库) │
                                        └───────┘
```

每个子系统都需要不同的编程语言和运行工具：

| 子系统 | 编程语言 | 需要的工具 | 用途 |
|--------|----------|------------|------|
| C++ 后端 | C++ | GCC, CMake, Drogon | 处理 HTTP 请求、业务逻辑 |
| Python AI Service | Python | Python3, pip, FastAPI | AI 文本分析、图像生成 |
| React 前端 | TypeScript | Node.js, npm, Vite | 用户界面 |
| 数据库 | SQL | PostgreSQL | 存储用户、绘本等数据 |

### 1.3 安装过程详解

#### 1.3.1 安装 Node.js（JavaScript 运行时）

**为什么需要 Node.js？**

前端项目使用 JavaScript/TypeScript 编写，Node.js 让我们可以在浏览器之外运行这些代码。npm（Node Package Manager）是 Node.js 自带的包管理器，用来安装前端依赖库（类似应用商店）。

**安装方式：使用 nvm（Node Version Manager）**

```bash
# 下载并安装 nvm（Node 版本管理器）
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash

# 重启终端后，安装 Node.js 18（长期支持版）
nvm install 18

# 验证安装
node --version    # 输出: v18.20.8
npm --version     # 输出: 10.8.2
```

**为什么用 nvm 而不是直接装 Node.js？**

nvm 可以让你在同一台电脑上安装多个版本的 Node.js，并随时切换。比如项目 A 需要 Node 16，项目 B 需要 Node 18，nvm 可以轻松管理。

#### 1.3.2 安装 Docker 和 Docker Compose

**什么是 Docker？**

Docker 是一个"容器化"工具。可以把应用和它需要的所有依赖打包成一个"容器"（像一个轻量级的虚拟机），保证在任何电脑上都能一致运行。

**为什么需要 Docker？**

- 开发时可以用 Docker 快速启动 PostgreSQL 和 Redis，不用手动安装配置
- 部署时保证所有人的运行环境一致
- `docker-compose` 可以一键启动所有服务（数据库 + 后端 + AI 服务 + 前端）

**安装过程：**

```bash
# 安装 Docker 官方 GPG 密钥（确保软件来源可信）
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | \
  sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# 添加 Docker 软件源
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.gpg] \
  https://download.docker.com/linux/ubuntu jammy stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list

# 安装 Docker Engine 和 Docker Compose
sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io \
  docker-buildx-plugin docker-compose-plugin

# 将当前用户加入 docker 组（免 sudo 使用 docker）
sudo usermod -aG docker pluto
```

**安装后的版本：**
- Docker: 29.8.1
- Docker Compose: 5.5.1

#### 1.3.3 安装 PostgreSQL（数据库）

**什么是 PostgreSQL？**

PostgreSQL 是一个开源的**关系型数据库**。就像 Excel 表格可以存储数据一样，数据库可以存储应用的所有数据（用户账号、绘本内容、生成任务等），但比 Excel 更强大、更安全、支持更多数据量。

**安装过程：**

```bash
# 安装 PostgreSQL 服务端
sudo apt install -y postgresql postgresql-contrib

# 启动数据库服务
sudo systemctl start postgresql
sudo systemctl enable postgresql  # 设置开机自启
```

**创建数据库和用户：**

```sql
-- 切换到 postgres 超级用户
sudo -u postgres psql

-- 创建专用用户（不要用 root 跑应用）
CREATE USER storycanvas WITH PASSWORD 'storycanvas20061104';

-- 创建数据库
CREATE DATABASE storycanvas OWNER storycanvas;

-- 授权
GRANT ALL PRIVILEGES ON DATABASE storycanvas TO storycanvas;
```

**为什么要创建专用用户？**

直接用 root 或 postgres 超级用户运行应用有安全风险。创建一个只有必要权限的专用用户，即使应用被攻击，损失也有限。这叫"最小权限原则"。

#### 1.3.4 创建 .env 配置文件

`.env` 文件用来存放项目的**环境变量**（配置信息）。不同环境（开发、测试、生产）可以用不同的 `.env` 文件。

```bash
cp .env.example .env
```

`.env` 文件内容：

```env
# 数据库连接信息
DB_HOST=localhost
DB_PORT=5432
DB_NAME=storycanvas
DB_USER=storycanvas
DB_PASSWORD=storycanvas20061104

# JWT 密钥（用于用户登录验证，随机生成的长字符串）
JWT_SECRET=24bd313c8ccbc4675f862cdfb5a3b547a12486c9346d38bea88b43f52b5fa3d2

# AI 服务地址
AI_SERVICE_URL=http://localhost:8000

# 各服务端口号
BACKEND_PORT=8080
AI_SERVICE_PORT=8000
FRONTEND_PORT=5173
```

**为什么用 .env 文件？**

配置信息（密码、密钥等）不应该写死在代码里，否则：
1. 代码泄露 → 密码也泄露
2. 不同环境要改代码才能切换配置

`.env` 文件通过 `.gitignore` 排除在版本控制之外，每个开发者各自维护自己的配置。

### 1.4 验证环境

Phase 0 完成后，所有工具的安装状态：

| 工具 | 版本 | 用途 | 安装方式 |
|------|------|------|----------|
| Git | 2.34.1 | 版本控制 | 系统自带 |
| GCC/G++ | 11.4.0 | C++ 编译器 | 系统自带 |
| CMake | 3.22.1 | C++ 构建工具 | 系统自带 |
| Drogon | (libdrogon.a) | C++ Web 框架 | 手动编译安装 |
| Python3 | 3.10.12 | Python 解释器 | 系统自带 |
| Node.js | 18.20.8 | JavaScript 运行时 | nvm 安装 |
| npm | 10.8.2 | JS 包管理器 | 随 Node.js |
| PostgreSQL | 14.24 | 数据库 | apt 安装 |
| Docker | 29.8.1 | 容器引擎 | apt 安装 |
| Docker Compose | 5.5.1 | 多容器编排 | apt 安装 |

---

## 二、Phase 1：项目骨架搭建

### 2.1 什么是项目骨架？

项目骨架就像**房子的地基和框架**。我们不急着装修（实现具体功能），先把墙壁、门窗、水电线路的框架搭好，确保：
1. 项目能编译/构建
2. 各模块之间能互相通信
3. 基础的请求能跑通（如健康检查接口）

### 2.2 C++ 后端（Backend）

#### 目录结构

```
backend/
├── CMakeLists.txt              # 构建配置文件（告诉编译器怎么编译项目）
├── main.cc                     # 程序入口（整个后端从这里启动）
├── config/
│   └── config.json             # Drogon 配置文件（端口、数据库连接等）
├── controllers/                # 控制器层（接收和处理 HTTP 请求）
│   ├── HealthController.h      # 健康检查控制器（头文件）
│   └── HealthController.cc     # 健康检查控制器（实现）
├── filters/                    # 过滤器/中间件（请求拦截处理）
│   ├── CorsFilter.h            # 跨域过滤器（头文件）
│   └── CorsFilter.cc           # 跨域过滤器（实现）
├── dto/                        # 数据传输对象（定义请求/响应格式）
│   └── ApiResponse.h           # 统一 API 响应格式
├── utils/                      # 工具类
│   ├── ConfigManager.h         # 配置管理器（读取环境变量）
│   └── ConfigManager.cc        # 配置管理器实现
├── services/                   # 业务逻辑层（后续开发填充）
├── models/                     # 数据模型层（后续开发填充）
├── repositories/               # 数据访问层（后续开发填充）
├── build/                      # 编译输出目录
│   └── storycanvas_backend     # 编译后的可执行文件
├── logs/                       # 日志目录
└── sql/                        # 数据库脚本目录
```

#### 各文件详解

**CMakeLists.txt** — 构建配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(storycanvas_backend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)    # 使用 C++17 标准

# 查找系统中安装的依赖库
find_package(Drogon REQUIRED)      # Web 框架
find_package(OpenSSL REQUIRED)     # 加密库（用于 JWT 等）
find_package(nlohmann_json REQUIRED)  # JSON 解析库
find_package(spdlog REQUIRED)      # 日志库
find_package(PostgreSQL REQUIRED)  # 数据库客户端

# 收集所有 .cc 源文件
file(GLOB_RECURSE SOURCES
    "controllers/*.cc"
    "filters/*.cc"
    "utils/*.cc"
    # ... 其他层的文件
)

# 编译成可执行文件
add_executable(${PROJECT_NAME} main.cc ${SOURCES})

# 链接依赖库
target_link_libraries(${PROJECT_NAME}
    ${DROGON_LIBRARIES}
    OpenSSL::SSL
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pq         # PostgreSQL 客户端库
    pthread    # 多线程库
)
```

**什么是 CMake？**

CMake 是一个"构建系统"。C++ 代码需要编译成机器码才能运行，CMake 负责：
1. 检查系统中有哪些依赖库
2. 确定哪些源文件需要编译
3. 调用编译器生成可执行文件

**main.cc** — 程序入口

```cpp
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include "utils/ConfigManager.h"

int main() {
    // 1. 从环境变量加载配置
    ConfigManager::init();

    // 2. 获取 Drogon 应用实例
    auto &app = drogon::app();

    // 3. 加载配置文件（端口、数据库等）
    app.loadConfigFile("./config/config.json");

    // 4. 启动 Web 服务器
    spdlog::info("StoryCanvas Backend 启动中...");
    app.run();  // 阻塞运行，监听 HTTP 请求

    return 0;
}
```

**controllers/HealthController** — 健康检查

```cpp
// HealthController.h
#pragma once
#include <drogon/HttpSimpleController.h>

class HealthController : public drogon::HttpSimpleController<HealthController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback
    ) override;

    // 路由注册：GET /health
    PATH_LIST_BEGIN
    PATH_ADD("/health", Get);
    PATH_LIST_END
};

// HealthController.cc
void HealthController::asyncHandleHttpRequest(...) {
    // 返回 JSON: {"code":200,"message":"StoryCanvas Backend is running"}
    auto resp = HttpResponse::newHttpJsonResponse(
        ApiResponse::success("StoryCanvas Backend is running")
    );
    resp->setStatusCode(k200OK);
    callback(resp);
}
```

**什么是"健康检查"？**

健康检查接口（`GET /health`）是最基础的 API，用来验证服务是否正常运行。就像医院的体检一样，调用这个接口返回 200 就说明后端活得好好的。

**filters/CorsFilter** — 跨域过滤器

```cpp
void CorsFilter::doFilter(const HttpRequestPtr &req, ...) {
    // 添加 CORS 头，允许前端跨域调用后端
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");

    // 如果是 OPTIONS 预检请求，直接返回 204
    if (req->getMethod() == Options) {
        resp->setStatusCode(k204NoContent);
        fcb(resp);
        return;
    }

    // 否则放行，继续处理请求
    fccb();
}
```

**什么是跨域（CORS）？**

浏览器出于安全考虑，默认禁止网页访问不同域名的接口。前端运行在 `localhost:5173`，后端运行在 `localhost:8080`，属于不同"源"。CORS 过滤器告诉浏览器："我允许这个前端访问我"。

**dto/ApiResponse.h** — 统一响应格式

```cpp
class ApiResponse {
public:
    static json success(const std::string &message) {
        return {{"code", 200}, {"message", message}};
    }
    static json error(int code, const std::string &message) {
        return {{"code", code}, {"message", message}};
    }
};
```

所有 API 返回统一的 JSON 格式：`{"code": 200, "message": "...", "data": {...}}`。这样前端可以统一处理响应，不用每个接口各写一套。

**utils/ConfigManager** — 配置管理器

```cpp
void ConfigManager::init() {
    // 从系统环境变量读取配置
    dbHost = getEnv("DB_HOST", "localhost");
    dbPort = std::stoi(getEnv("DB_PORT", "5432"));
    jwtSecret = getEnv("JWT_SECRET", "");
    // ...
}
```

将 `.env` 文件中的配置读入 C++ 代码，这样代码里不用硬编码数据库密码等敏感信息。

#### 编译与运行

```bash
cd backend
mkdir -p build && cd build
cmake ..        # 生成构建文件
make -j$(nproc) # 并行编译

# 启动后端
./storycanvas_backend

# 验证（另一个终端）
curl http://localhost:8080/health
# 返回: {"code":200,"message":"StoryCanvas Backend is running"}
```

---

### 2.3 Python AI Service

#### 目录结构

```
ai-service/
├── main.py                         # 启动入口（uvicorn 运行 FastAPI）
├── requirements.txt                # Python 依赖列表
├── .env.example                    # 环境变量示例
├── app/
│   ├── __init__.py
│   ├── main.py                     # FastAPI 应用创建
│   ├── api/                        # API 路由层
│   │   ├── __init__.py
│   │   └── v1/                     # API 版本 v1
│   │       ├── __init__.py
│   │       ├── router.py           # 路由注册
│   │       └── health.py           # 健康检查接口
│   ├── models/                     # 数据模型
│   │   ├── __init__.py
│   │   └── schemas.py              # Pydantic 模型（请求/响应结构）
│   ├── services/                   # 业务逻辑（后续开发）
│   ├── providers/                  # AI 服务提供商（后续开发）
│   │   ├── llm/                    # 大语言模型
│   │   └── image/                  # 图像生成
│   ├── prompts/                    # Prompt 模板（后续开发）
│   └── utils/                      # 工具函数
│       ├── __init__.py
│       ├── config.py               # 配置管理（Pydantic Settings）
│       ├── logger.py               # 日志配置
│       └── exceptions.py           # 自定义异常
├── tests/                          # 测试文件
│   ├── __init__.py
│   ├── test_services/
│   ├── test_providers/
│   └── test_api/
└── venv/                           # Python 虚拟环境（自动生成）
```

#### 各文件详解

**requirements.txt** — Python 依赖

```
fastapi==0.104.1           # Web 框架
uvicorn[standard]==0.24.0  # ASGI 服务器（运行 FastAPI）
pydantic==2.5.2            # 数据验证
httpx==0.25.2              # HTTP 客户端（调用外部 API）
openai==1.3.7              # OpenAI SDK
python-dotenv==1.0.0       # 读取 .env 文件
```

**什么是 Python 虚拟环境？**

虚拟环境（`venv`）为每个项目创建独立的 Python 包空间，避免不同项目的依赖互相冲突。

```bash
python3 -m venv venv        # 创建虚拟环境
source venv/bin/activate     # 激活（之后 pip install 只影响这个项目）
pip install -r requirements.txt  # 安装依赖
```

**app/main.py** — FastAPI 应用

```python
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.api.v1.router import api_router

def create_app() -> FastAPI:
    app = FastAPI(
        title="StoryCanvas AI Service",
        version="1.0.0"
    )

    # CORS 中间件
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["http://localhost:5173"],
        allow_methods=["*"],
        allow_headers=["*"],
    )

    # 注册路由（所有 API 以 /api/v1 开头）
    app.include_router(api_router, prefix="/api/v1")

    return app

app = create_app()
```

**什么是 FastAPI？**

FastAPI 是一个现代的 Python Web 框架，特点是：
1. **快**：性能接近 Go 和 Node.js
2. **自动生成文档**：访问 `/docs` 可以看到交互式 API 文档
3. **类型安全**：用 Pydantic 模型验证请求数据

**app/api/v1/health.py** — 健康检查

```python
from fastapi import APIRouter

router = APIRouter()

@router.get("/")
async def health_check():
    return {
        "status": "ok",
        "service": "StoryCanvas AI Service",
        "version": "1.0.0"
    }
```

**app/models/schemas.py** — 数据模型

```python
from pydantic import BaseModel, Field
from enum import Enum

class ArtStyle(str, Enum):
    CARTOON = "cartoon"       # 卡通风格
    WATERCOLOR = "watercolor" # 水彩风格
    ANIME = "anime"           # 动漫风格

class GenerationRequest(BaseModel):
    """AI 图像生成请求"""
    text: str              # 用户输入的故事文本
    style: ArtStyle        # 艺术风格
    book_id: str = None    # 关联的绘本 ID
```

Pydantic 模型的作用：
1. 自动验证请求数据（类型、必填字段等）
2. 自动转换数据类型
3. 自动生成 API 文档

**app/utils/config.py** — 配置管理

```python
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    OPENAI_API_KEY: str = ""
    OPENAI_MODEL: str = "gpt-3.5-turbo"
    STORAGE_PATH: str = "./storage"

    class Config:
        env_file = ".env"   # 自动从 .env 文件读取

settings = Settings()
```

Pydantic Settings 自动将 `.env` 文件中的环境变量映射到类属性，代码中直接用 `settings.OPENAI_API_KEY` 即可。

#### 启动与验证

```bash
cd ai-service
source venv/bin/activate
python main.py

# 验证（另一个终端）
curl http://localhost:8000/api/v1/health/
# 返回: {"status":"ok","service":"StoryCanvas AI Service","version":"1.0.0"}
```

---

### 2.4 React 前端（Frontend）

#### 目录结构

```
frontend/
├── index.html                  # HTML 模板（Vite 用这个作为入口）
├── package.json                # 项目配置和依赖列表
├── vite.config.ts              # Vite 构建工具配置
├── tsconfig.json               # TypeScript 编译配置
├── tailwind.config.js          # Tailwind CSS 配置
├── postcss.config.js           # PostCSS 配置
└── src/
    ├── main.tsx                # React 入口（挂载根组件）
    ├── App.tsx                 # 根组件（路由配置）
    ├── vite-env.d.ts           # Vite 类型声明
    ├── api/
    │   └── client.ts           # Axios HTTP 客户端封装
    ├── stores/
    │   └── authStore.ts        # 认证状态管理（Zustand）
    ├── pages/
    │   └── home/
    │       └── Home.tsx        # 首页组件
    ├── components/             # 可复用组件（后续开发）
    │   ├── common/             # 通用组件
    │   ├── editor/             # 编辑器组件
    │   ├── ai/                 # AI 相关组件
    │   ├── book/               # 绘本组件
    │   └── layout/             # 布局组件
    ├── hooks/                  # 自定义 Hooks（后续开发）
    ├── types/                  # TypeScript 类型定义（后续开发）
    ├── utils/                  # 工具函数（后续开发）
    └── styles/
        └── globals.css         # 全局样式（Tailwind）
```

#### 各文件详解

**package.json** — 项目配置

```json
{
  "name": "storycanvas-frontend",
  "dependencies": {
    "react": "^18.2.0",          // UI 框架
    "react-dom": "^18.2.0",      // DOM 渲染
    "react-router-dom": "^6.20.0", // 路由管理
    "antd": "^5.12.0",           // UI 组件库（Ant Design）
    "zustand": "^4.4.7",         // 状态管理
    "axios": "^1.6.2"            // HTTP 客户端
  },
  "devDependencies": {
    "vite": "^5.0.0",            // 构建工具
    "typescript": "^5.3.2",      // TypeScript
    "tailwindcss": "^3.3.6"      // 原子化 CSS
  }
}
```

**什么是这些库？**

- **React**：构建用户界面的 JavaScript 库。把页面拆分成一个个"组件"，像搭积木一样组合。
- **Ant Design**：现成的 UI 组件库（按钮、表格、弹窗等），不用自己写样式。
- **Zustand**：轻量级状态管理。多个组件需要共享数据时用（如用户登录状态）。
- **Axios**：发送 HTTP 请求的库，比浏览器原生的 fetch 更好用。
- **Vite**：前端构建工具，开发时自动热更新（改代码后浏览器自动刷新）。
- **TypeScript**：给 JavaScript 加上类型系统的"超集"，减少运行时错误。

**vite.config.ts** — 构建配置

```typescript
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',  // 开发时代理到后端
        changeOrigin: true,
      },
    },
  },
})
```

**什么是代理（proxy）？**

开发时前端运行在 `5173` 端口，后端运行在 `8080` 端口。浏览器会阻止跨域请求。Vite 的代理功能让前端的 `/api` 请求转发到后端，绕过跨域限制。

**src/main.tsx** — React 入口

```tsx
import React from 'react'
import ReactDOM from 'react-dom/client'
import App from './App'
import './styles/globals.css'   // 引入 Tailwind CSS

// 把 App 组件挂载到 HTML 的 #root 元素
ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
)
```

**src/App.tsx** — 路由配置

```tsx
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { useAuthStore } from './stores/authStore'
import Home from './pages/home/Home'

// 路由守卫：未登录则跳转到登录页
const ProtectedRoute = ({ children }: { children: React.ReactNode }) => {
  const { isAuthenticated } = useAuthStore()
  return isAuthenticated ? <>{children}</> : <Navigate to="/login" />
}

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={
          <ProtectedRoute><Home /></ProtectedRoute>
        } />
        <Route path="*" element={<Navigate to="/" />} />
      </Routes>
    </BrowserRouter>
  )
}
```

**什么是路由？**

路由决定了浏览器地址栏的 URL 对应显示哪个页面。比如：
- `/` → 首页
- `/login` → 登录页
- `/books` → 绘本列表
- `/editor/123` → 编辑绘本

**src/stores/authStore.ts** — 认证状态管理

```typescript
import { create } from 'zustand'
import { persist } from 'zustand/middleware'

interface AuthState {
  user: User | null
  accessToken: string | null
  isAuthenticated: boolean
  login: (response: AuthResponse) => void
  logout: () => void
}

export const useAuthStore = create<AuthState>()(
  persist(
    (set) => ({
      user: null,
      accessToken: null,
      isAuthenticated: false,

      login: (response) => set({
        user: response.user,
        accessToken: response.access_token,
        isAuthenticated: true,
      }),

      logout: () => set({
        user: null,
        accessToken: null,
        isAuthenticated: false,
      }),
    }),
    { name: 'auth-storage' }  // 持久化到 localStorage
  )
)
```

`persist` 中间件把登录状态保存到浏览器的 localStorage，刷新页面后不用重新登录。

**src/api/client.ts** — HTTP 客户端

```typescript
import axios from 'axios'
import { useAuthStore } from '../stores/authStore'

const client = axios.create({
  baseURL: 'http://localhost:8080/api/v1',
  timeout: 30000,
})

// 请求拦截器：自动带上 JWT Token
client.interceptors.request.use((config) => {
  const { accessToken } = useAuthStore.getState()
  if (accessToken) {
    config.headers.Authorization = `Bearer ${accessToken}`
  }
  return config
})

// 响应拦截器：401 时自动退出登录
client.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      useAuthStore.getState().logout()
      window.location.href = '/login'
    }
    return Promise.reject(error)
  }
)
```

#### 安装与验证

```bash
cd frontend
npm install              # 安装所有依赖
npx tsc --noEmit         # TypeScript 类型检查
npx vite build           # 生产构建

# 开发模式
npm run dev              # 启动开发服务器
# 浏览器访问 http://localhost:5173
```

---

### 2.5 数据库初始化

**sql/init.sql** 定义了所有数据表：

```sql
-- users 表：存储用户账号信息
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),  -- 用户唯一 ID
    username VARCHAR(50) NOT NULL UNIQUE,            -- 用户名（唯一）
    email VARCHAR(255) NOT NULL UNIQUE,              -- 邮箱（唯一）
    password_hash VARCHAR(255) NOT NULL,             -- 密码哈希（不是明文！）
    avatar VARCHAR(500),                             -- 头像路径
    is_active BOOLEAN DEFAULT true,                  -- 账号是否激活
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- books 表：存储绘本信息
CREATE TABLE books (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,  -- 外键关联用户
    title VARCHAR(200) NOT NULL,                     -- 绘本标题
    description TEXT,                                -- 绘本描述
    cover_image VARCHAR(500),                        -- 封面图片
    status VARCHAR(20) DEFAULT 'draft'               -- 状态：草稿/已发布/已归档
);
```

**什么是 UUID？**

UUID（通用唯一标识符）是一个 128 位的随机 ID，格式如 `550e8400-e29b-41d4-a716-446655440000`。相比自增整数 ID，UUID 的优势是：
1. 全球唯一，不会冲突
2. 不暴露数据量（用户猜不到你有多少数据）
3. 可以在不同数据库间合并

**什么是外键（FOREIGN KEY）？**

外键是表与表之间的关联。`books.user_id` 引用 `users.id`，表示"这本书属于哪个用户"。`ON DELETE CASCADE` 意味着删除用户时，自动删除他的所有绘本。

**7 张表的关系：**

```
users ──1:N──▶ books ──1:N──▶ book_pages ──1:N──▶ page_elements
  │                              ▲
  ├──1:N──▶ characters           │
  │                              │
  └──1:N──▶ generation_tasks ────┘
                │
                └──1:N──▶ generated_images
```

执行初始化：

```bash
PGPASSWORD=storycanvas20061104 psql -h localhost -U storycanvas \
  -d storycanvas -f sql/init.sql
```

---

## 三、项目整体结构

```
StoryCanvas/
├── .env                          # 环境变量（不入库）
├── .env.example                  # 环境变量模板
├── .gitignore                    # Git 忽略规则
├── docker-compose.yml            # Docker 多服务编排
├── README.md                     # 项目说明
├── docs/                         # 设计文档
├── sql/
│   └── init.sql                  # 数据库初始化脚本
├── storage/                      # 文件存储目录
├── backend/                      # C++ 后端
│   ├── CMakeLists.txt
│   ├── main.cc
│   ├── config/
│   ├── controllers/
│   ├── filters/
│   ├── dto/
│   ├── utils/
│   ├── services/                 # 后续开发
│   ├── models/                   # 后续开发
│   ├── repositories/             # 后续开发
│   └── build/                    # 编译产物
├── ai-service/                   # Python AI 服务
│   ├── main.py
│   ├── requirements.txt
│   ├── app/
│   │   ├── main.py
│   │   ├── api/
│   │   ├── models/
│   │   ├── services/             # 后续开发
│   │   ├── providers/            # 后续开发
│   │   ├── prompts/              # 后续开发
│   │   └── utils/
│   ├── tests/
│   └── venv/                     # Python 虚拟环境
└── frontend/                     # React 前端
    ├── package.json
    ├── vite.config.ts
    ├── index.html
    └── src/
        ├── main.tsx
        ├── App.tsx
        ├── api/
        ├── stores/
        ├── pages/
        ├── components/           # 后续开发
        ├── hooks/                # 后续开发
        ├── types/                # 后续开发
        └── utils/                # 后续开发
```

---

## 四、常用启动命令速查

### 启动后端

```bash
cd ~/StoryCanvas/backend/build
./storycanvas_backend
# 访问 http://localhost:8080/health
```

### 启动 AI Service

```bash
cd ~/StoryCanvas/ai-service
source venv/bin/activate
python main.py
# 访问 http://localhost:8000/api/v1/health/
```

### 启动前端（开发模式）

```bash
cd ~/StoryCanvas/frontend
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh"
npm run dev
# 访问 http://localhost:5173
```

### 启动数据库

```bash
sudo systemctl start postgresql
sudo systemctl status postgresql
```

### 数据库操作

```bash
# 连接数据库
PGPASSWORD=storycanvas20061104 psql -h localhost -U storycanvas -d storycanvas

# 查看所有表
\dt

# 查看表结构
\d users
```

---

## 五、遇到的问题与解决方案

### 问题 1：CMakeLists.txt 文件名错误

**现象**：`cmake ..` 找不到构建配置

**原因**：文件名写成了 `CMakeList.txt`（少了 `s`），正确应为 `CMakeLists.txt`

**解决**：
```bash
mv CMakeList.txt CMakeLists.txt
```

### 问题 2：C++ 源文件扩展名不匹配

**现象**：CMake 配置成功但编译时找不到源文件

**原因**：文件后缀是 `.cpp`，但 CMakeLists.txt 中 glob 规则匹配的是 `*.cc`

**解决**：将 `.cpp` 文件重命名为 `.cc`
```bash
mv main.cpp main.cc
mv HealthController.cpp HealthController.cc
```

### 问题 3：Drogon 头文件找不到

**现象**：`fatal error: drogon/HttpSimpleFilter.h: No such file or directory`

**原因**：Drogon 没有 `HttpSimpleFilter.h`，只有 `HttpFilter.h`

**解决**：
```cpp
// 错误
#include <drogon/HttpSimpleFilter.h>
// 正确
#include <drogon/HttpFilter.h>
```

### 问题 4：HttpSimpleController 需要路由注册

**现象**：`error: 'initPathRouting' is not a member of 'HealthController'`

**原因**：`HttpSimpleController` 要求子类必须定义路由

**解决**：
```cpp
class HealthController : public drogon::HttpSimpleController<HealthController> {
    PATH_LIST_BEGIN
    PATH_ADD("/health", Get);
    PATH_LIST_END
};
```

### 问题 5：TypeScript 报错 `import.meta.env`

**现象**：`error TS2339: Property 'env' does not exist on type 'ImportMeta'`

**原因**：缺少 Vite 的类型声明

**解决**：创建 `src/vite-env.d.ts`
```typescript
/// <reference types="vite/client" />
```

### 问题 6：数据库密码不匹配

**现象**：`psql: FATAL: password authentication failed for user "storycanvas"`

**原因**：`.env` 中的密码与 PostgreSQL 实际密码不一致

**解决**：
```sql
ALTER USER storycanvas WITH PASSWORD '新的密码';
```

---

## 六、技术栈速查表

| 技术 | 版本 | 用途 | 学习资源 |
|------|------|------|----------|
| C++ | 17 | 后端语言 | cppreference.com |
| Drogon | - | C++ Web 框架 | drogonframework.org |
| CMake | 3.22 | C++ 构建系统 | cmake.org |
| Python | 3.10 | AI 服务语言 | python.org |
| FastAPI | 0.104 | Python Web 框架 | fastapi.tiangolo.com |
| React | 18 | 前端 UI 框架 | react.dev |
| TypeScript | 5.3 | JS 类型系统 | typescriptlang.org |
| Vite | 5.0 | 前端构建工具 | vitejs.dev |
| Tailwind CSS | 3.3 | 原子化 CSS | tailwindcss.com |
| Zustand | 4.4 | 状态管理 | zustand-demo.pmnd.rs |
| PostgreSQL | 14 | 关系型数据库 | postgresql.org |
| Docker | 29 | 容器化 | docker.com |

---

*本文档记录了 StoryCanvas 项目 Phase 0 和 Phase 1 的完整开发过程。*
*下一步将进入 Phase 2：用户认证模块开发。*
