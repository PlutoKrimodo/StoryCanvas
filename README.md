# StoryCanvas

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## 项目简介

StoryCanvas 是一个基于生成式 AI 的儿童绘本智能创作与图像生成系统。用户可以通过自然语言描述，利用 AI 技术自动生成绘本插画，并支持生成结果的预览与管理。

### 核心功能

- **用户管理**: 注册、登录、JWT 双 Token 认证与刷新、密码 bcrypt 加密存储
- **绘本管理**: 创建、查看、更新、删除绘本项目（分页 + 状态筛选 + 归属隔离）
- **AI 图像生成**: 文本解析（角色/场景/物品/动作/风格）、Prompt 优化、图像生成
- **任务管理**: 后端内存队列 + 工作线程异步生成，任务状态落库与轮询查询
- **结果预览**: 生成结果展示、放大、单张下载与重新生成
- **保存到绘本**: 把生成图按页绑定到绘本（`book_pages.image_id`）
- **PDF 导出**: 绘本按页合成多页 PDF（含中文页标题）前端生成下载
- **生成历史**: 历史记录查看与参数复用、多风格选择、尺寸 / 随机种子调优

> **离线可跑**：AI 服务支持真实厂商（DeepSeek / 豆包·Seedream / 通义万相）与 **Mock 双模式**，
> 通过 `LLM_PROVIDER` / `IMAGE_PROVIDER` 一键切换；`mock` 会生成真实的 PNG，
> 因此「解析 → 生成 → 落盘 → 预览 → 导出 PDF」整条链路无外网也能演示。


### 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | React, TypeScript, Ant Design, Tailwind CSS |
| 后端 | C++17, Drogon Framework |
| AI Service | Python, FastAPI |
| AI 服务商 | DeepSeek（LLM）/ 豆包·Seedream · 通义万相（图像，Mock 可降级） |
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
| [开发记录](docs/开发记录.md) | **持续更新的开发记录**（Phase 0 起，每阶段追加实现过程与踩坑） |

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
# 注意：STORAGE_PATH 需让后端与 AI 服务指向同一目录（模板已统一为 ../storage）

# AI Service 读取自己目录下的 .env（服务商密钥在这里配置）
cp ai-service/.env.example ai-service/.env

# 4. 启动数据库
docker-compose up -d postgres redis

# 5. 初始化数据库
psql -h localhost -U storycanvas -d storycanvas -f sql/init.sql

# 6. 启动后端（构建产物名为 storycanvas_backend）
cd backend
cp ../.env .env                          # 后端在 backend/ 下读取 .env
mkdir -p build && cd build
cmake .. && make -j$(nproc)
cd .. && ./build/storycanvas_backend     # http://localhost:8080/health

# 7. 启动 AI Service
cd ai-service
source venv/bin/activate
python -m venv venv && ./venv/bin/pip install -r requirements.txt   # 首次
python main.py                           # http://localhost:8000/api/v1/health/
# 演示故障兜底（无需外网）：LLM_PROVIDER=mock IMAGE_PROVIDER=mock python main.py

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
│   ├── app/providers/       #   LLM（DeepSeek/Mock）与图像（豆包/通义万相/Mock）Provider
│   ├── app/services/        #   文本分析 / Prompt 优化 / 图像生成 / 一站式编排
│   ├── app/prompts/         #   分析 Prompt 与 5 套风格模板
│   └── tests/               #   pytest 单元测试
├── frontend/                # React 前端
│   └── src/                 #   api / stores / hooks / components(ai,book) / pages / utils
├── sql/init.sql             # 数据库初始化脚本
├── scripts/                 # 环境初始化与验收脚本
│   ├── setup.sh             #   环境初始化
│   ├── verify_m1..m4.sh     #   Phase 2 / 3 / 4 / 5 验收脚本
│   └── start_all.sh         #   一键启动本地三端
├── nginx/nginx.conf         # 反向代理配置（HTTPS 段默认注释）
├── storage/                 # 生成结果（后端与 AI 服务共享；按 users/{uid}/books/{bid}/ 分层）
├── docker-compose.yml       # 容器编排
└── .env.example             # 环境变量模板（公共配置唯一来源）
```

---

## 开发阶段

| 阶段 | 内容 | 状态 | 时间 |
|------|------|------|------|
| Phase 0 | 环境搭建 | ✅ 已完成 | 1-2 天 |
| Phase 1 | 项目骨架 | ✅ 已完成 | 2-3 天 |
| Phase 2 | 用户系统（含加密存储） | ✅ 已完成 | 3-4 天 |
| Phase 3 | 绘本管理 | ✅ 已完成 | 2-3 天 |
| Phase 4 | AI Service 基础（文本解析） | ✅ 已完成 | 3-4 天 |
| Phase 5 | AI 图像生成（核心） | ✅ 已完成 | 3-4 天 |
| Phase 6 | 生成预览与历史 | ✅ 已完成 | 2-3 天 |
| Phase 10 | 测试完善 | ⬜ 未开始 | 3-4 天 |
| Phase 11 | Docker 部署 | ⬜ 未开始 | 1-2 天 |
| Phase 12 | 课程报告 | ⬜ 未开始 | 3-5 天 |

**总时间估算**: 约 20-29 天（精简 MVP 10-12 天）

> 原 Phase 6 在线编辑器、Phase 7 多页绘本已放弃（属在线编辑子系统）；**PDF 导出以「轻量多页（前端生成）」形式加回**。权威排期见 [项目实施方案](docs/16-项目实施方案.md)。

---

## MVP 功能

### 必须实现（✅ 已完成）

- 用户注册/登录/登出
- JWT 认证与 Token 刷新
- 用户信息与密码管理（bcrypt 加密存储）
- 创建绘本、查看绘本列表与详情
- AI 文本解析与 Prompt 优化
- AI 图像生成（真实厂商 + Mock 降级）
- 生成结果预览
- 重新生成 / 取消任务
- 保存到绘本（页容器绑定 `image_id`）
- PDF 导出（轻量多页，前端生成）

### 加分项（✅ 已完成，Phase 6）

- 生成历史（列表 + 参数复用）
- 多艺术风格（5 套风格模板）
- 参数调优面板（尺寸 / 随机种子）
- 单张图片下载

### 第二阶段（未开始）

- 角色管理与角色一致性生成
- 游客优先（Guest-first，见 `docs/TODO.md` T-1）
- Redis 任务队列、并发与性能压测

---

## 测试

```bash
# 运行后端单元测试（GoogleTest，覆盖 utils 层）
cd backend/build
cmake .. && make -j"$(nproc)" && ctest --output-on-failure

# 运行 AI Service 单元测试（pytest）
cd ai-service
./venv/bin/python -m pytest

# 运行前端类型检查与单元测试
cd frontend
npx tsc --noEmit && npm test

# 端到端验收脚本（需三端已启动）
./scripts/verify_m1.sh   # Phase 2 用户系统
./scripts/verify_m2.sh   # Phase 3 绘本管理
./scripts/verify_m3.sh   # Phase 4 文本解析
./scripts/verify_m4.sh   # Phase 5 图像生成闭环
```
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
