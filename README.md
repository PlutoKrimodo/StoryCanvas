# StoryCanvas

## 基于生成式 AI 的儿童绘本智能创作与在线编辑系统

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## 项目简介

StoryCanvas 是一个基于生成式 AI 的儿童绘本智能创作与在线编辑系统。用户可以通过自然语言描述，利用 AI 技术自动生成绘本插画，并提供在线编辑功能，支持多页绘本的创建和导出。

### 核心功能

- **用户管理**: 注册、登录、JWT 认证
- **绘本管理**: 创建、编辑、删除绘本项目
- **AI 图像生成**: 文本解析、Prompt 优化、图像生成
- **在线编辑器**: 基于 Fabric.js 的 Canvas 编辑
- **页面管理**: 多页绘本支持
- **导出功能**: 支持导出为 PDF

### 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | React, TypeScript, Ant Design, Fabric.js |
| 后端 | C++17, Drogon Framework |
| AI Service | Python, FastAPI |
| 数据库 | PostgreSQL |
| 缓存 | Redis (可选) |
| 部署 | Docker, Docker Compose |

---

## 项目文档

本项目包含完整的设计文档，位于 `docs/` 目录：

| 文档 | 说明 |
|------|------|
| [项目方案总览](docs/00-项目方案总览.md) | 项目整体概述 |
| [软件需求规格说明书](docs/01-SRS-软件需求规格说明书.md) | 详细需求分析 |
| [系统总体架构设计](docs/02-系统总体架构设计.md) | 系统架构设计 |
| [数据库设计](docs/03-数据库设计.md) | 数据库 ER 图和表结构 |
| [REST API 设计](docs/04-REST-API设计.md) | 完整 API 接口设计 |
| [C++ 后端项目结构](docs/05-CppBackend项目结构.md) | 后端架构设计 |
| [Python AI Service 结构](docs/06-PythonAIService结构.md) | AI 服务架构设计 |
| [前端架构设计](docs/07-前端架构设计.md) | 前端架构设计 |
| [AI 异步任务机制](docs/08-AI异步任务机制.md) | 异步任务设计 |
| [文件存储方案](docs/09-文件存储方案.md) | 文件存储设计 |
| [安全机制设计](docs/10-安全机制设计.md) | 安全机制设计 |
| [测试方案](docs/11-测试方案.md) | 测试策略和用例 |
| [开发阶段与 MVP 规划](docs/12-开发阶段与MVP规划.md) | 开发计划 |
| [课程报告与演示](docs/13-课程报告与演示.md) | 报告结构和演示流程 |
| [WSL 开发环境](docs/14-WSL开发环境.md) | 开发环境搭建 |
| [Git 项目组织](docs/15-Git项目组织.md) | Git 工作流 |

---

## 快速开始

### 环境要求

- WSL 2 + Ubuntu 22.04
- GCC/G++ 11+
- CMake 3.16+
- Python 3.10+
- Node.js 18+
- PostgreSQL 15
- Docker (可选)

### 开发环境搭建

```bash
# 1. 克隆项目
git clone https://github.com/your-username/StoryCanvas.git
cd StoryCanvas

# 2. 运行环境搭建脚本
chmod +x scripts/setup.sh
./scripts/setup.sh

# 3. 配置环境变量
cp .env.example .env
# 编辑 .env 文件

# 4. 启动数据库
docker-compose up -d postgres redis

# 5. 初始化数据库
psql -h localhost -U storycanvas -d storycanvas -f sql/init.sql

# 6. 启动后端
cd backend
mkdir build && cd build
cmake .. && make -j$(nproc)
./StoryCanvasBackend

# 7. 启动 AI Service
cd ai-service
source venv/bin/activate
uvicorn app.main:app --reload --port 8000

# 8. 启动前端
cd frontend
npm install
npm run dev
```

### Docker 部署

```bash
# 构建并启动所有服务
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

---

## 项目结构

```
StoryCanvas/
├── docs/                    # 项目文档
├── backend/                 # C++ 后端
├── ai-service/              # Python AI Service
├── frontend/                # React 前端
├── sql/                     # 数据库脚本
├── scripts/                 # 脚本工具
├── storage/                 # 文件存储
├── tests/                   # 测试
├── docker-compose.yml       # Docker 编排
└── .env.example             # 环境变量示例
```

---

## 开发阶段

| 阶段 | 内容 | 时间 |
|------|------|------|
| Phase 0 | 环境搭建 | 1-2 天 |
| Phase 1 | 项目骨架 | 2-3 天 |
| Phase 2 | 用户系统 | 3-4 天 |
| Phase 3 | 绘本管理 | 2-3 天 |
| Phase 4 | AI Service | 3-4 天 |
| Phase 5 | AI 生成 | 3-4 天 |
| Phase 6 | 在线编辑器 | 5-7 天 |
| Phase 7 | 多页绘本 | 2-3 天 |
| Phase 8 | 异步任务 | 2-3 天 |
| Phase 9 | 导出功能 | 2-3 天 |
| Phase 10 | 测试完善 | 3-4 天 |
| Phase 11 | Docker 部署 | 1-2 天 |
| Phase 12 | 课程报告 | 3-5 天 |

**总时间估算**: 32-47 天

---

## MVP 功能

### 必须实现

- 用户注册/登录
- JWT 认证
- 创建绘本
- AI 文本解析
- AI 图像生成
- 图片预览
- 添加图片到页面
- 移动/缩放图片
- 页面管理
- 导出 PDF

### 第二阶段

- 旋转图片
- 添加文字
- 撤销/重做
- 生成历史
- 角色管理
- 多种导出格式

---

## 测试

```bash
# 运行后端测试
cd backend/build
ctest

# 运行 AI Service 测试
cd ai-service
pytest tests/

# 运行前端测试
cd frontend
npm test
```

---

## 课程报告

报告结构详见 [课程报告与演示](docs/13-课程报告与演示.md)。

### 报告目录

1. 绪论
2. 需求分析
3. 系统总体设计
4. 数据库设计
5. 系统详细设计
6. AI 模块设计
7. 系统实现
8. 系统测试
9. 总结与展望

---

## 演示流程

### 5 分钟演示

1. 开场介绍（30 秒）
2. 用户注册（30 秒）
3. 用户登录（20 秒）
4. 创建绘本（30 秒）
5. AI 图像生成（2 分钟）
6. 图片编辑（1 分钟）
7. 添加文字（30 秒）
8. 保存和导出（30 秒）
9. 总结（30 秒）

---

## 贡献

欢迎提交 Issue 和 Pull Request！

---

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情

---

## 致谢

- [Drogon Framework](https://github.com/drogonframework/drogon)
- [FastAPI](https://fastapi.tiangolo.com/)
- [React](https://react.dev/)
- [Fabric.js](http://fabricjs.com/)
- [Ant Design](https://ant.design/)
