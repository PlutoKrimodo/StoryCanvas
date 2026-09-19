# Git 项目组织

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

**版本**: 1.0  
**日期**: 2026-09-17  
**作者**: StoryCanvas 开发团队

---

## 1. 项目目录结构

### 1.1 完整目录结构

```
StoryCanvas/
├── .gitignore                      # Git 忽略配置
├── README.md                       # 项目说明
├── .env.example                    # 环境变量示例
├── docker-compose.yml              # Docker 编排配置
├── LICENSE                         # 开源许可证
│
├── docs/                           # 文档目录
│   ├── 01-SRS-软件需求规格说明书.md
│   ├── 02-系统总体架构设计.md
│   ├── 03-数据库设计.md
│   ├── 04-REST-API设计.md
│   ├── 05-CppBackend项目结构.md
│   ├── 06-PythonAIService结构.md
│   ├── 07-前端架构设计.md
│   ├── 08-AI异步任务机制.md
│   ├── 09-文件存储方案.md
│   ├── 10-安全机制设计.md
│   ├── 11-测试方案.md
│   ├── 12-开发阶段与MVP规划.md
│   ├── 13-课程报告与演示.md
│   ├── 14-WSL开发环境.md
│   └── 15-Git项目组织.md
│
├── backend/                        # C++ 后端
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── config/
│   │   ├── config.json
│   │   └── config.json.example
│   ├── controllers/
│   ├── services/
│   ├── models/
│   ├── repositories/
│   ├── filters/
│   ├── dto/
│   ├── utils/
│   ├── sql/
│   ├── tests/
│   └── main.cc
│
├── ai-service/                     # Python AI Service
│   ├── app/
│   │   ├── api/
│   │   ├── services/
│   │   ├── providers/
│   │   ├── models/
│   │   ├── prompts/
│   │   └── utils/
│   ├── tests/
│   ├── requirements.txt
│   ├── requirements-dev.txt
│   ├── pyproject.toml
│   └── main.py
│
├── frontend/                       # React 前端
│   ├── public/
│   ├── src/
│   │   ├── api/
│   │   ├── components/
│   │   ├── hooks/
│   │   ├── pages/
│   │   ├── stores/
│   │   ├── types/
│   │   ├── utils/
│   │   ├── styles/
│   │   ├── App.tsx
│   │   └── main.tsx
│   ├── package.json
│   ├── tsconfig.json
│   ├── vite.config.ts
│   └── tailwind.config.js
│
├── sql/                            # 数据库脚本
│   ├── init.sql
│   ├── migrations/
│   └── seeds/
│
├── scripts/                        # 脚本工具
│   ├── setup.sh                    # 环境搭建脚本
│   ├── start.sh                    # 启动脚本
│   ├── stop.sh                     # 停止脚本
│   └── deploy.sh                   # 部署脚本
│
├── storage/                        # 文件存储
│   ├── users/
│   ├── temp/
│   └── public/
│
├── tests/                          # 测试
│   ├── backend/
│   ├── ai-service/
│   ├── frontend/
│   └── integration/
│
└── .github/                        # GitHub 配置
    └── workflows/
        └── ci.yml
```

---

## 2. .gitignore 配置

### 2.1 完整 .gitignore 文件

```gitignore
# ==================== 通用 ====================
# 操作系统文件
.DS_Store
.DS_Store?
._*
.Spotlight-V100
.Trashes
ehthumbs.db
Thumbs.db

# 编辑器配置
.vscode/
.idea/
*.swp
*.swo
*~
.project
.classpath
.settings/

# ==================== C++ 后端 ====================
# 编译产物
backend/build/
backend/out/
*.o
*.obj
*.exe
*.out
*.app
*.a
*.so
*.dylib

# CMake 产物
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile
compile_commands.json

# ==================== Python AI Service ====================
# 虚拟环境
ai-service/venv/
ai-service/.venv/
ai-service/env/

# Python 缓存
__pycache__/
*.py[cod]
*$py.class
*.so
.Python
build/
develop-eggs/
dist/
downloads/
eggs/
.eggs/
lib/
lib64/
parts/
sdist/
var/
wheels/
*.egg-info/
.installed.cfg
*.egg

# Jupyter Notebook
.ipynb_checkpoints

# ==================== 前端 ====================
# 依赖
frontend/node_modules/
frontend/.pnp
frontend/.pnp.js

# 构建产物
frontend/dist/
frontend/build/
frontend/.cache/

# 测试覆盖率
frontend/coverage/

# ==================== 环境配置 ====================
# 环境变量
.env
.env.local
.env.*.local

# ==================== 文件存储 ====================
# 用户上传文件
storage/users/*
!storage/users/.gitkeep

# 临时文件
storage/temp/*
!storage/temp/.gitkeep

# 导出文件
storage/exports/*
!storage/exports/.gitkeep

# ==================== 日志 ====================
*.log
logs/
npm-debug.log*
yarn-debug.log*
yarn-error.log*

# ==================== Docker ====================
docker-compose.override.yml

# ==================== 其他 ====================
*.bak
*.tmp
*.temp
.cache/
tmp/
temp/
```

### 2.2 各模块 .gitignore

#### backend/.gitignore

```gitignore
# 编译产物
build/
out/
*.o
*.obj

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
```

#### ai-service/.gitignore

```gitignore
# 虚拟环境
venv/
.venv/
env/

# Python 缓存
__pycache__/
*.py[cod]
*.egg-info/
```

#### frontend/.gitignore

```gitignore
# 依赖
node_modules/

# 构建
dist/
build/
.cache/
```

---

## 3. Git 分支策略

### 3.1 分支模型

```
main                    # 主分支（稳定版本）
  │
  ├── develop           # 开发分支（最新开发进度）
  │     │
  │     ├── feature/user-auth      # 功能分支
  │     ├── feature/book-management
  │     ├── feature/ai-generation
  │     └── feature/editor
  │
  ├── release/v1.0      # 发布分支
  │
  └── hotfix/xxx        # 热修复分支
```

### 3.2 分支命名规范

| 分支类型 | 命名格式 | 说明 |
|---------|---------|------|
| 主分支 | `main` | 生产环境代码 |
| 开发分支 | `develop` | 开发环境代码 |
| 功能分支 | `feature/xxx` | 新功能开发 |
| 发布分支 | `release/v1.0` | 版本发布 |
| 热修复 | `hotfix/xxx` | 紧急修复 |

### 3.3 分支操作流程

#### 功能开发流程

```bash
# 1. 从 develop 创建功能分支
git checkout develop
git pull origin develop
git checkout -b feature/user-auth

# 2. 开发功能
# ... 编写代码 ...

# 3. 提交代码
git add .
git commit -m "feat: 实现用户注册功能"

# 4. 推送到远程
git push origin feature/user-auth

# 5. 创建 Pull Request
# 在 GitHub 上创建 PR，合并到 develop

# 6. 删除功能分支
git checkout develop
git pull origin develop
git branch -d feature/user-auth
git push origin --delete feature/user-auth
```

#### 发布流程

```bash
# 1. 从 develop 创建发布分支
git checkout develop
git checkout -b release/v1.0

# 2. 版本号更新
# ... 更新版本号 ...

# 3. 提交
git commit -am "chore: 版本 v1.0"

# 4. 合并到 main
git checkout main
git merge release/v1.0
git tag -a v1.0 -m "版本 v1.0"
git push origin main --tags

# 5. 合并回 develop
git checkout develop
git merge release/v1.0
git push origin develop

# 6. 删除发布分支
git branch -d release/v1.0
git push origin --delete release/v1.0
```

---

## 4. Git Commit 规范

### 4.1 Commit Message 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 4.2 Type 类型

| 类型 | 说明 | 示例 |
|------|------|------|
| feat | 新功能 | feat: 添加用户注册功能 |
| fix | 修复 bug | fix: 修复登录失败问题 |
| docs | 文档更新 | docs: 更新 API 文档 |
| style | 代码格式（不影响功能） | style: 格式化代码 |
| refactor | 重构 | refactor: 重构用户服务 |
| test | 测试相关 | test: 添加单元测试 |
| chore | 构建/工具相关 | chore: 更新依赖 |
| perf | 性能优化 | perf: 优化查询性能 |

### 4.3 Scope 范围

| 范围 | 说明 |
|------|------|
| auth | 认证相关 |
| user | 用户相关 |
| book | 绘本相关 |
| page | 页面相关 |
| ai | AI 服务相关 |
| book | 绘本相关 |
| db | 数据库相关 |
| api | API 相关 |
| frontend | 前端相关 |
| backend | 后端相关 |

### 4.4 示例

```bash
# 简单 commit
git commit -m "feat: 添加用户注册功能"

# 带范围的 commit
git commit -m "feat(auth): 实现 JWT 认证"

# 带详细说明的 commit
git commit -m "feat(book): 实现绘本创建功能

- 支持创建绘本
- 支持设置标题和描述
- 支持设置封面图片

Closes #123"
```

---

## 5. Git 工作流

### 5.1 日常开发流程

```bash
# 1. 更新 develop 分支
git checkout develop
git pull origin develop

# 2. 创建功能分支
git checkout -b feature/my-feature

# 3. 开发并提交
git add .
git commit -m "feat: 实现功能"

# 4. 推送
git push origin feature/my-feature

# 5. 创建 PR 并合并
# 在 GitHub 上操作

# 6. 删除本地分支
git checkout develop
git branch -d feature/my-feature
```

### 5.2 代码审查流程

1. 创建 Pull Request
2. 填写 PR 描述
3. 等待代码审查
4. 根据反馈修改
5. 合并到目标分支

### 5.3 PR 模板

```markdown
## 描述
简要描述这个 PR 的内容

## 类型
- [ ] 新功能
- [ ] Bug 修复
- [ ] 文档更新
- [ ] 重构
- [ ] 其他

## 测试
- [ ] 已添加单元测试
- [ ] 已手动测试
- [ ] 测试通过

## 截图（如果有）
添加相关截图

## 相关 Issue
Closes #123
```

---

## 6. 标签管理

### 6.1 版本标签

```bash
# 创建标签
git tag -a v1.0.0 -m "版本 v1.0.0"

# 推送标签
git push origin v1.0.0

# 列出标签
git tag -l

# 删除标签
git tag -d v1.0.0
git push origin --delete v1.0.0
```

### 6.2 版本号规范

采用语义化版本号：`MAJOR.MINOR.PATCH`

- **MAJOR**: 主版本号（不兼容的 API 修改）
- **MINOR**: 次版本号（向下兼容的功能新增）
- **PATCH**: 补丁号（向下兼容的问题修正）

示例：`v1.2.3`

---

## 7. Git Hooks

### 7.1 使用 Husky（前端）

```bash
# 安装 husky
npm install husky --save-dev

# 初始化
npx husky install

# 添加 pre-commit hook
npx husky add .husky/pre-commit "npm run lint"

# 添加 commit-msg hook
npx husky add .husky/commit-msg "npx commitlint --edit"
```

### 7.2 使用 pre-commit（Python）

```bash
# 安装 pre-commit
pip install pre-commit

# 创建 .pre-commit-config.yaml
# 配置钩子

# 安装钩子
pre-commit install
```

### 7.3 pre-commit 配置示例

```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/psf/black
    rev: 22.3.0
    hooks:
      - id: black
        language_version: python3

  - repo: https://github.com/pycqa/isort
    rev: 5.10.1
    hooks:
      - id: isort

  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.0.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
```

---

## 8. 远程仓库配置

### 8.1 GitHub 仓库设置

```bash
# 添加远程仓库
git remote add origin https://github.com/username/StoryCanvas.git

# 查看远程仓库
git remote -v

# 推送代码
git push -u origin main
```

### 8.2 分支保护

在 GitHub 上设置分支保护规则：

1. 进入仓库设置
2. 选择 Branches
3. 添加规则：
   - Branch name pattern: `main`
   - Require pull request reviews: ✓
   - Require status checks: ✓
   - Require branches to be up to date: ✓

### 8.3 协作者管理

```bash
# 添加协作者
# 在 GitHub 仓库设置中添加

# 设置权限
- Admin: 管理仓库设置
- Write: 推送代码
- Read: 只读权限
```

---

## 9. 常见 Git 操作

### 9.1 撤销操作

```bash
# 撤销最后一次 commit（保留修改）
git reset --soft HEAD~1

# 撤销最后一次 commit（丢弃修改）
git reset --hard HEAD~1

# 撤销已推送的 commit
git revert HEAD
git push origin main
```

### 9.2 解决冲突

```bash
# 合并冲突
git merge feature-branch

# 解决冲突后
git add .
git commit -m "fix: 解决合并冲突"
```

### 9.3 查看历史

```bash
# 查看提交历史
git log --oneline --graph

# 查看文件修改历史
git log -p filename

# 查看某个 commit
git show commit-id
```

---

## 10. 项目初始化脚本

### 10.1 setup.sh

```bash
#!/bin/bash

# StoryCanvas 项目初始化脚本

set -e

echo "🚀 开始初始化 StoryCanvas 项目..."

# 检查依赖
echo "📦 检查依赖..."
command -v git >/dev/null 2>&1 || { echo "错误: 需要安装 git"; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "错误: 需要安装 cmake"; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "错误: 需要安装 python3"; exit 1; }
command -v node >/dev/null 2>&1 || { echo "错误: 需要安装 node"; exit 1; }

# 初始化 Git
echo "🔧 初始化 Git..."
git init
git remote add origin https://github.com/username/StoryCanvas.git

# 创建 .env 文件
echo "📝 创建 .env 文件..."
if [ ! -f .env ]; then
    cp .env.example .env
    echo "请编辑 .env 文件配置环境变量"
fi

# 安装后端依赖
echo "📦 安装后端依赖..."
cd backend
mkdir -p build
cd build
cmake ..
cd ../..

# 安装 AI Service 依赖
echo "📦 安装 AI Service 依赖..."
cd ai-service
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cd ..

# 安装前端依赖
echo "📦 安装前端依赖..."
cd frontend
npm install
cd ..

echo "✅ 项目初始化完成！"
echo ""
echo "下一步："
echo "1. 编辑 .env 文件配置环境变量"
echo "2. 启动数据库服务"
echo "3. 运行数据库初始化脚本"
echo "4. 启动后端服务"
echo "5. 启动 AI Service"
echo "6. 启动前端开发服务器"
```

---

**文档结束**
