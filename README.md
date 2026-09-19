# StoryCanvas

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## 项目简介

StoryCanvas 是一个基于生成式 AI 的儿童绘本智能创作与图像生成系统。用户可以通过自然语言描述，利用 AI 技术自动生成绘本插画，并支持生成结果的预览与管理。

### 核心功能

- **用户管理**: 注册、登录、JWT 认证、密码加密存储（bcrypt）
- **绘本管理**: 创建、查看、更新、删除绘本项目
- **AI 图像生成**: 文本解析、Prompt 优化、图像生成
- **任务管理**: 异步生成任务与状态查询
- **结果预览**: 生成结果展示与重新生成

> 说明：原「在线编辑器 / 页面管理 / PDF 导出」属在线编辑子系统，本轮已放弃。

### 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | React, TypeScript, Ant Design, Tailwind CSS |
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
| [项目实施方案](docs/16-项目实施方案.md) | **权威实施方案（v2.0，含范围收敛与加密存储设计）** |
| [开发记录](docs/Phase0-Phase1-开发记录.md) | Phase 0 / Phase 1 实施记录 |

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

# 3. 配置环境变量（至少填写 DB_PASSWORD / JWT_SECRET / AI_SERVICE_API_KEY）
cp .env.example .env
# 用 openssl rand 生成随机密钥填入，不要沿用示例值

# 4. 启动数据库
docker-compose up -d postgres redis

# 5. 初始化数据库
psql -h localhost -U storycanvas -d storycanvas -f sql/init.sql

# 6. 启动后端（构建产物名为 storycanvas_backend）
cd backend
mkdir -p build && cd build
cmake .. && make -j$(nproc)
./storycanvas_backend                    # http://localhost:8080/health

# 7. 启动 AI Service
cd ai-service
source venv/bin/activate
uvicorn app.main:app --reload --port 8000   # http://localhost:8000/api/v1/health/

# 8. 启动前端
cd frontend
npm install
npm run dev                              # http://localhost:5173
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
├── docs/                    # 项目文档（权威实施方案见 docs/16）
├── backend/                 # C++ 后端
│   ├── controllers/         #   控制器
│   ├── services/            #   业务逻辑层
│   ├── repositories/        #   数据访问层
│   ├── models/              #   数据模型
│   ├── filters/             #   CORS 等过滤器
│   ├── utils/               #   配置 / JWT / 密码哈希 / 加密工具
│   └── config/config.json   #   仅非敏感默认值，密钥走环境变量
├── ai-service/              # Python AI Service（文本解析 / Prompt 优化 / 图像生成）
├── frontend/                # React 前端
├── sql/init.sql             # 数据库初始化脚本
├── scripts/setup.sh         # 环境初始化脚本
├── nginx/nginx.conf         # 反向代理配置（HTTPS 段默认注释）
├── storage/                 # 生成结果与临时文件
├── docker-compose.yml       # 容器编排
└── .env.example             # 环境变量模板（公共配置唯一来源）
```

---

## 开发阶段

| 阶段 | 内容 | 时间 |
|------|------|------|
| Phase 0 | 环境搭建 | 1-2 天 |
| Phase 1 | 项目骨架 | 2-3 天 |
| Phase 2 | 用户系统（含加密存储） | 3-4 天 |
| Phase 3 | 绘本管理 | 2-3 天 |
| Phase 4 | AI Service 基础 | 3-4 天 |
| Phase 5 | AI 图像生成（核心） | 3-4 天 |
| Phase 6 | 生成预览与历史 | 2-3 天 |
| Phase 10 | 测试完善 | 3-4 天 |
| Phase 11 | Docker 部署 | 1-2 天 |
| Phase 12 | 课程报告 | 3-5 天 |

**总时间估算**: 约 20-29 天（精简 MVP 10-12 天）

> 原 Phase 6 在线编辑器、Phase 7 多页绘本、Phase 9 导出功能已放弃（属在线编辑子系统）。权威排期见 [项目实施方案](docs/16-项目实施方案.md)。

---

## MVP 功能

### 必须实现

- 用户注册/登录/登出
- JWT 认证与 Token 刷新
- 用户信息与密码管理（bcrypt 加密存储）
- 创建绘本、查看绘本列表与详情
- AI 文本解析与 Prompt 优化
- AI 图像生成
- 生成结果预览
- 重新生成

### 第二阶段

- 生成历史
- 角色管理与角色一致性
- 多艺术风格与参数调优
- 单张图片下载

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
4. 创建绘本（20 秒）
5. AI 图像生成（2 分 30 秒）
6. 结果预览与重新生成（40 秒）
7. 保存到绘本 / 下载图片（20 秒）
8. 总结（30 秒）

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
- [Ant Design](https://ant.design/)
