# 开发阶段与 MVP 规划

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

**版本**: 2.0  
**日期**: 2026-09-19  
**作者**: StoryCanvas 开发团队

> **v2.0 修订**：范围收敛为**用户管理**与**智能图像生成**两大子系统（重点为后者），**在线编辑子系统整体放弃**。
>
> 本文件中的 **Phase 6（在线编辑器）**、**Phase 7（多页绘本）**、**Phase 9（导出功能）已作废**；最新阶段划分与排期以 `docs/16-项目实施方案.md` 第 8、9 章为准。

---

## 1. 开发阶段总览

### 1.1 阶段划分

```
Phase 0: 开发环境搭建（1-2 天）
    ↓
Phase 1: 项目骨架搭建（2-3 天）
    ↓
Phase 2: 用户系统（3-4 天，含加密存储）
    ↓
Phase 3: 绘本项目管理（2-3 天）
    ↓
Phase 4: AI Service 基础（3-4 天）
    ↓
Phase 5: AI 图像生成（3-4 天）★ 本轮核心
    ↓
Phase 6: 生成预览与历史（2-3 天）
    ↓
Phase 10: 测试完善（3-4 天）
    ↓
Phase 11: Docker 部署（1-2 天）
    ↓
Phase 12: 课程报告（3-5 天）
```

> ~~Phase 6 在线编辑器~~、~~Phase 7 多页绘本~~、~~Phase 9 导出功能~~ 已作废。

### 1.2 时间估算

| 阶段 | 依赖 | 时间估算 | 累计时间 |
|------|------|---------|---------|
| Phase 0 | 无 | 1-2 天 | 1-2 天 |
| Phase 1 | Phase 0 | 2-3 天 | 3-5 天 |
| Phase 2 用户系统（含加密存储） | Phase 1 | 3-4 天 | 6-9 天 |
| Phase 3 绘本项目管理 | Phase 2 | 2-3 天 | 8-12 天 |
| Phase 4 AI Service 基础 | Phase 3 | 3-4 天 | 11-16 天 |
| Phase 5 AI 图像生成 | Phase 4 | 3-4 天 | 14-20 天 |
| Phase 6 生成预览与历史 | Phase 5 | 2-3 天 | 16-23 天 |
| Phase 10 测试完善 | Phase 5/6 | 3-4 天 | 19-27 天 |
| Phase 11 Docker 部署 | Phase 10 | 1-2 天 | 20-29 天 |
| Phase 12 课程报告 | Phase 11 | 3-5 天 | 23-34 天 |

---

## 2. 各阶段详细设计

### 2.1 Phase 0: 开发环境搭建

#### 目标
搭建完整的开发环境，确保所有工具和服务可用。

#### 要实现的功能
- WSL 环境配置
- C++ 开发环境（GCC, CMake, Drogon）
- Python 开发环境（Python 3.10+, venv）
- 数据库环境（PostgreSQL）
- 前端开发环境（Node.js, npm）
- Docker 环境
- VS Code 配置

#### 涉及的代码目录
```
StoryCanvas/
├── .gitignore
├── README.md
├── .env.example
├── docker-compose.yml
└── docs/
```

#### 数据库变化
无

#### API
无

#### 测试
- 验证 C++ 编译环境
- 验证 Python 环境
- 验证数据库连接
- 验证前端构建

#### 完成标准
- 所有开发工具安装完成
- 示例项目编译运行成功
- 数据库连接正常

---

### 2.2 Phase 1: 项目骨架搭建

#### 目标
创建前后端项目骨架，建立基础架构。

#### 要实现的功能
- C++ 后端项目结构
- Python AI Service 项目结构
- 前端项目结构
- 基础配置文件
- 数据库初始化脚本
- 基础 API 框架

#### 涉及的代码目录
```
StoryCanvas/
├── backend/
│   ├── CMakeLists.txt
│   ├── config/
│   ├── controllers/
│   ├── services/
│   ├── models/
│   └── main.cc
├── ai-service/
│   ├── app/
│   ├── requirements.txt
│   └── main.py
├── frontend/
│   ├── package.json
│   ├── src/
│   └── vite.config.ts
└── sql/
    └── init.sql
```

#### 数据库变化
- 创建所有核心表

#### API
- Health check API

#### 测试
- 项目编译测试
- API 基础测试

#### 完成标准
- 三个项目都能正常启动
- Health check API 正常响应

---

### 2.3 Phase 2: 用户系统

#### 目标
实现完整的用户认证和管理功能。

#### 要实现的功能
- 用户注册
- 用户登录
- JWT 认证
- 用户信息管理
- 密码修改
- **密码 bcrypt 加盐哈希存储（加密存储）**
- **敏感配置经环境变量注入，禁止硬编码**

#### 涉及的代码目录
```
backend/
├── controllers/AuthController.cc
├── controllers/UserController.cc
├── services/UserService.cc
├── repositories/UserRepository.cc
├── models/User.h
├── filters/JwtFilter.cc
├── utils/JwtUtils.cc
└── utils/PasswordUtils.cc

frontend/
├── src/pages/auth/Login.tsx
├── src/pages/auth/Register.tsx
├── src/stores/authStore.ts
├── src/api/auth.ts
└── src/hooks/useAuth.ts
```

#### 数据库变化
- users 表

#### API
```
POST /api/v1/auth/register
POST /api/v1/auth/login
POST /api/v1/auth/refresh
POST /api/v1/auth/logout
GET /api/v1/users/me
PUT /api/v1/users/me
PUT /api/v1/users/me/password
```

#### 测试
- 注册功能测试
- 登录功能测试
- JWT 验证测试
- 权限控制测试

#### 完成标准
- 用户可以注册、登录
- JWT 认证正常工作
- 用户信息可以修改
- 数据库中密码为 bcrypt 哈希、非明文且不可逆

---

### 2.4 Phase 3: 绘本项目管理

#### 目标
实现绘本的创建、查看、更新、删除功能。

#### 要实现的功能
- 创建绘本
- 获取绘本列表
- 获取绘本详情
- 更新绘本信息
- 删除绘本

#### 涉及的代码目录
```
backend/
├── controllers/BookController.cc
├── services/BookService.cc
├── repositories/BookRepository.cc
└── models/Book.h

frontend/
├── src/pages/books/BookList.tsx
├── src/pages/books/BookDetail.tsx
├── src/components/book/BookCard.tsx
├── src/stores/bookStore.ts
└── src/api/books.ts
```

#### 数据库变化
- books 表

#### API
```
POST /api/v1/books
GET /api/v1/users/me/books
GET /api/v1/books/{book_id}
PUT /api/v1/books/{book_id}
DELETE /api/v1/books/{book_id}
```

#### 测试
- 创建绘本测试
- 获取绘本列表测试
- 权限控制测试

#### 完成标准
- 用户可以创建、查看、修改、删除绘本
- 只能操作自己的绘本

---

### 2.5 Phase 4: AI Service 基础

#### 目标
搭建 Python AI Service 基础架构，实现文本分析和 Prompt 优化。

#### 要实现的功能
- 文本分析服务
- Prompt 优化服务
- LLM Provider 抽象
- Mock Provider（测试用）

#### 涉及的代码目录
```
ai-service/
├── app/
│   ├── services/text_analyzer.py
│   ├── services/prompt_optimizer.py
│   ├── providers/llm/base.py
│   ├── providers/llm/mock_provider.py
│   ├── prompts/text_analysis.py
│   └── prompts/prompt_optimization.py
└── tests/
```

#### 数据库变化
无

#### API
```
POST /api/v1/analysis/parse
```

#### 测试
- 文本分析测试
- Prompt 优化测试
- Mock Provider 测试

#### 完成标准
- 文本分析服务正常工作
- Prompt 优化服务正常工作
- Mock Provider 可用于测试

---

### 2.6 Phase 5: AI 图片生成

#### 目标
实现完整的 AI 图片生成流程。

#### 要实现的功能
- 图像生成服务
- Image Provider 抽象
- 生成任务管理
- 生成结果存储

#### 涉及的代码目录
```
backend/
├── controllers/GenerationController.cc
├── services/GenerationService.cc
├── services/AiClient.cc
├── repositories/GenerationTaskRepository.cc
└── models/GenerationTask.h

ai-service/
├── app/services/image_generator.py
├── app/services/task_manager.py
├── app/providers/image/base.py
├── app/providers/image/mock_provider.py
├── app/api/v1/generation.py
└── tests/
```

#### 数据库变化
- generation_tasks 表
- generated_images 表

#### API
```
POST /api/v1/generation
GET /api/v1/generation/{task_id}
GET /api/v1/generation/history
```

#### 测试
- 图像生成测试
- 任务状态管理测试
- 异步任务测试

#### 完成标准
- 可以创建生成任务
- 任务状态可以查询
- 生成结果可以获取

---

### 2.7 Phase 6: 在线编辑器（**已作废**）

> **本阶段整体放弃（时间原因）。** 以下内容仅作历史留档，不再执行。
> 替代阶段为「生成预览与历史」，详见 `docs/16-项目实施方案.md` 第 8.6 节。

#### 目标
实现基于 Fabric.js 的在线编辑器。

#### 要实现的功能
- Canvas 画布
- 图片添加和编辑
- 文字添加和编辑
- 元素移动、缩放、旋转
- 撤销/重做
- 属性面板

#### 涉及的代码目录
```
frontend/
├── src/components/editor/Canvas.tsx
├── src/components/editor/Toolbar.tsx
├── src/components/editor/PropertyPanel.tsx
├── src/components/editor/ImageElement.tsx
├── src/components/editor/TextElement.tsx
├── src/pages/editor/Editor.tsx
├── src/stores/editorStore.ts
├── src/hooks/useCanvas.ts
└── src/api/images.ts

backend/
├── controllers/PageController.cc
├── controllers/ElementController.cc
├── controllers/ImageController.cc
├── services/PageService.cc
├── services/ElementService.cc
├── services/ImageService.cc
└── models/PageElement.h
```

#### 数据库变化
- book_pages 表
- page_elements 表

#### API
```
POST /api/v1/books/{book_id}/pages
GET /api/v1/pages/{page_id}
PUT /api/v1/pages/{page_id}
DELETE /api/v1/pages/{page_id}

POST /api/v1/pages/{page_id}/elements
PUT /api/v1/elements/{element_id}
DELETE /api/v1/elements/{element_id}

POST /api/v1/images/upload
GET /api/v1/images/{image_id}
```

#### 测试
- 页面管理测试
- 元素操作测试
- 图片上传测试
- 编辑器功能测试

#### 完成标准
- 可以创建和编辑页面
- 可以添加图片和文字
- 可以移动、缩放、旋转元素
- 支持撤销/重做

---

### 2.8 Phase 7: 多页绘本（**已作废**）

> **本阶段放弃（时间原因）。** `book_pages` 降级为页容器，仅用于承载生成的图片，不提供多页排版编辑。

#### 目标
支持多页绘本的创建和管理。

#### 要实现的功能
- 页面列表
- 页面排序
- 页面预览
- 页面切换

#### 涉及的代码目录
```
frontend/
├── src/components/book/PageList.tsx
├── src/components/book/PageThumbnail.tsx
└── src/hooks/usePages.ts

backend/
├── services/PageService.cc
└── controllers/PageController.cc
```

#### 数据库变化
- book_pages 表增加排序字段

#### API
```
GET /api/v1/books/{book_id}/pages
PUT /api/v1/books/{book_id}/pages/reorder
```

#### 测试
- 页面排序测试
- 多页管理测试

#### 完成标准
- 可以创建多个页面
- 可以对页面排序
- 可以切换页面编辑

---

### 2.9 Phase 8: 异步任务/Redis（可选）

#### 目标
实现更完善的异步任务机制。

#### 要实现的功能
- 任务队列
- 任务状态轮询
- 超时处理
- 重试机制

#### 涉及的代码目录
```
backend/
├── services/GenerationService.cc
└── utils/RedisClient.cc

ai-service/
├── app/services/task_manager.py
└── app/api/v1/generation.py
```

#### 数据库变化
无

#### API
- 增强的任务状态查询

#### 测试
- 异步任务测试
- 超时处理测试
- 并发测试

#### 完成标准
- 异步任务可靠执行
- 支持超时和重试

---

### 2.10 Phase 9: 导出功能（**已作废**）

> **PDF 合成导出依赖排版能力，本轮放弃。** 导出降级为「单张图片下载」，列为加分项。

#### 目标
实现绘本导出功能。

#### 要实现的功能
- 导出为 PDF
- 导出为 PNG/JPEG
- 导出选项配置

#### 涉及的代码目录
```
backend/
├── controllers/ExportController.cc
├── services/ExportService.cc
└── utils/ImageProcessor.cc

frontend/
├── src/components/export/ExportDialog.tsx
└── src/api/export.ts
```

#### 数据库变化
无

#### API
```
POST /api/v1/books/{book_id}/export
GET /api/v1/export/{task_id}
```

#### 测试
- PDF 导出测试
- 图片导出测试

#### 完成标准
- 可以导出为 PDF
- 可以导出为图片

---

### 2.11 Phase 10: 测试完善

#### 目标
完善测试用例，提高测试覆盖率。

#### 要实现的功能
- 单元测试完善
- API 测试完善
- 集成测试
- 异常测试
- 性能测试

#### 涉及的代码目录
```
tests/
├── test_auth_controller.cc
├── test_user_service.cc
├── test_api/
├── test_services/
└── test_providers/
```

#### 数据库变化
无

#### API
无

#### 测试
- 所有测试用例

#### 完成标准
- 测试覆盖率达到 80%
- 所有核心功能都有测试

---

### 2.12 Phase 11: Docker 部署

#### 目标
实现 Docker 容器化部署。

#### 要实现的功能
- Dockerfile 编写
- Docker Compose 配置
- 环境变量管理
- 数据持久化

#### 涉及的代码目录
```
StoryCanvas/
├── docker-compose.yml
├── backend/Dockerfile
├── ai-service/Dockerfile
├── frontend/Dockerfile
└── .env.example
```

#### 数据库变化
无

#### API
无

#### 测试
- Docker 构建测试
- 服务启动测试

#### 完成标准
- 可以通过 Docker Compose 启动所有服务
- 服务之间可以正常通信

---

### 2.13 Phase 12: 课程报告

#### 目标
完成课程结课报告。

#### 要实现的功能
- 报告撰写
- 图表制作
- 演示材料准备

#### 涉及的代码目录
```
docs/
├── report/
│   ├── 第一章-绪论.md
│   ├── 第二章-需求分析.md
│   ├── 第三章-系统设计.md
│   ├── ...
│   └── 第九章-总结.md
└── slides/
    └── presentation.pdf
```

#### 数据库变化
无

#### API
无

#### 测试
- 演示流程测试

#### 完成标准
- 完成所有章节
- 准备好演示材料

---

## 3. MVP（最小可行产品）定义

### 3.1 MVP 必须实现的功能

#### 核心功能
1. **用户注册/登录**
   - 用户可以注册账号
   - 用户可以登录
   - JWT 认证

2. **绘本项目管理**
   - 创建绘本
   - 查看绘本列表
   - 查看绘本详情

3. **基础 AI 图像生成**
   - 输入故事文本
   - AI 解析文本
   - 生成图像
   - 查看生成结果

4. **用户信息与密码管理**
   - 查看/修改个人资料
   - 修改密码（bcrypt 加密存储）

5. **生成结果预览与重新生成**
   - 预览生成的图像
   - 基于同/新参数重新生成
   - 保存到绘本

### 3.2 MVP 可以放弃的功能

- 角色管理/角色一致性
- 生成历史
- 多艺术风格
- Redis 异步任务
- 单张图片下载
- 并发测试
- 性能测试
- **在线编辑子系统全部功能（已放弃）**

### 3.3 MVP 加分项

- 更好的 UI 设计
- 更多的艺术风格
- 生成历史记录
- 单张图片下载

---

## 4. MVP 开发时间表

### 4.1 精简 MVP（推荐，10-12 天）

```
Day 1-2: 环境搭建 + 项目骨架
Day 3-4: 用户系统（含加密存储）
Day 5-6: 绘本项目管理
Day 7-8: AI Service 基础（文本解析 + Prompt 优化）
Day 9-11: AI 图像生成闭环（核心）
Day 12: 生成预览增强 + 测试与修复
```

### 4.2 一周 MVP（极简版）

```
Day 1: 环境搭建 + 项目骨架
Day 2: 用户系统（登录/注册 + 密码哈希）
Day 3: 绘本管理
Day 4: AI Service（文本解析）
Day 5: AI 图像生成
Day 6: 结果预览 + 保存
Day 7: 测试 + 演示准备
```

---

## 5. 功能优先级矩阵

### 5.1 功能分类

| 功能 | MVP | Phase 2 | 加分项 | 说明 |
|------|-----|---------|--------|------|
| 用户注册/登录/登出 | ✅ | | | 必须 |
| JWT 认证与刷新 | ✅ | | | 必须 |
| 用户信息/密码管理（加密存储） | ✅ | | | 必须 |
| 创建绘本 | ✅ | | | 必须 |
| 查看绘本列表 | ✅ | | | 必须 |
| AI 文本解析 | ✅ | | | 必须 |
| Prompt 优化 | ✅ | | | 必须 |
| AI 图像生成 | ✅ | | | 必须 |
| 生成结果预览 | ✅ | | | 必须 |
| 重新生成 | ✅ | | | 必须 |
| 生成历史 | | ✅ | | 第二阶段 |
| 角色一致性 | | ✅ | | 第二阶段 |
| 多艺术风格 | | | ✅ | 加分项 |
| 单张图片下载 | | | ✅ | 加分项 |
| ~~图片/文字编辑、页面管理、PDF 导出~~ | | | | **已放弃** |

### 5.2 技术实现建议

#### MVP 阶段
- 使用简单的内存队列处理 AI 任务
- 使用本地文件系统存储
- 简单的 UI 设计
- 基本的错误处理

#### 第二阶段
- 添加 Redis 缓存
- 完善错误处理
- 优化 UI/UX
- 扩展生成能力（多风格、角色一致性）

---

## 6. 风险评估

### 6.1 技术风险

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| Drogon 学习曲线陡峭 | 高 | 中 | 提前学习，参考示例 |
| AI API 不稳定/限流 | 高 | 中 | 使用 Mock Provider 测试 |
| 图像生成耗时/成本高 | 中 | 中 | 异步任务 + 频次上限 |
| 加密库集成（bcrypt/OpenSSL） | 中 | 中 | 骨架阶段先验证编译链接 |
| WSL 环境问题 | 中 | 低 | 使用 Docker 部署 |

### 6.2 时间风险

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 功能范围过大 | 高 | 高 | 严格控制 MVP 范围 |
| 测试时间不足 | 中 | 中 | 提前编写测试 |
| 报告时间不足 | 中 | 中 | 边开发边记录 |

---

## 7. 开发建议

### 7.1 代码管理

1. 使用 Git 进行版本控制
2. 每个功能一个分支
3. 定期提交代码
4. 代码审查

### 7.2 开发顺序

1. 先完成后端 API
2. 再完成前端界面
3. 最后集成测试

### 7.3 文档记录

1. 记录设计决策
2. 记录遇到的问题
3. 记录解决方案
4. 为报告积累素材

---

**文档结束**
