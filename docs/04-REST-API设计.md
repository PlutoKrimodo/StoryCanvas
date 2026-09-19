# REST API 设计

## 基于生成式 AI 的儿童绘本智能创作与图像生成系统

**版本**: 2.0  
**日期**: 2026-09-19  
**作者**: StoryCanvas 开发团队

> **v2.0 修订**：范围收敛为**用户管理**与**智能图像生成**两大子系统，**在线编辑子系统整体放弃**。
> 已移除：页面模块、页面元素模块、绘本导出模块、图片上传接口。权威方案见 `docs/16-项目实施方案.md`。

---

## 1. API 概述

### 1.1 基础信息

- **Base URL**: `http://localhost:8080/api/v1`
- **认证方式**: JWT Bearer Token
- **Content-Type**: application/json
- **字符编码**: UTF-8

### 1.2 通用响应格式

```json
{
  "code": 200,
  "message": "success",
  "data": {}
}
```

### 1.3 错误响应格式

```json
{
  "code": 400,
  "message": "错误描述",
  "errors": [
    {
      "field": "字段名",
      "message": "具体错误信息"
    }
  ]
}
```

### 1.4 HTTP 状态码

| 状态码 | 说明 |
|--------|------|
| 200 | 成功 |
| 201 | 创建成功 |
| 204 | 删除成功 |
| 400 | 请求参数错误 |
| 401 | 未认证 |
| 403 | 权限不足 |
| 404 | 资源不存在 |
| 409 | 资源冲突 |
| 422 | 数据验证失败 |
| 500 | 服务器内部错误 |

---

## 2. 认证模块 (Authentication)

### 2.1 用户注册

**POST** `/api/v1/auth/register`

**描述**: 注册新用户账号

**请求体**:
```json
{
  "username": "john_doe",
  "email": "john@example.com",
  "password": "Password123!"
}
```

**响应**:
```json
{
  "code": 201,
  "message": "注册成功",
  "data": {
    "user": {
      "id": "uuid",
      "username": "john_doe",
      "email": "john@example.com",
      "avatar": null,
      "created_at": "2026-09-17T10:00:00Z"
    },
    "access_token": "eyJ...",
    "refresh_token": "eyJ..."
  }
}
```

**错误响应**:
- 400: 请求参数错误
- 409: 用户名或邮箱已存在

### 2.2 用户登录

**POST** `/api/v1/auth/login`

**描述**: 用户登录获取 Token

**请求体**:
```json
{
  "email": "john@example.com",
  "password": "Password123!"
}
```

**响应**:
```json
{
  "code": 200,
  "message": "登录成功",
  "data": {
    "user": {
      "id": "uuid",
      "username": "john_doe",
      "email": "john@example.com",
      "avatar": "url",
      "created_at": "2026-09-17T10:00:00Z"
    },
    "access_token": "eyJ...",
    "refresh_token": "eyJ..."
  }
}
```

**错误响应**:
- 400: 请求参数错误
- 401: 邮箱或密码错误

### 2.3 刷新 Token

**POST** `/api/v1/auth/refresh`

**描述**: 使用 Refresh Token 获取新的 Access Token

**请求体**:
```json
{
  "refresh_token": "eyJ..."
}
```

**响应**:
```json
{
  "code": 200,
  "message": "刷新成功",
  "data": {
    "access_token": "eyJ...",
    "refresh_token": "eyJ..."
  }
}
```

**错误响应**:
- 401: Refresh Token 无效或已过期

### 2.4 用户退出

**POST** `/api/v1/auth/logout`

**描述**: 用户退出登录（需要认证）

**认证**: 需要 JWT Token

**响应**:
```json
{
  "code": 200,
  "message": "退出成功"
}
```

---

## 3. 用户模块 (User)

### 3.1 获取用户信息

**GET** `/api/v1/users/me`

**描述**: 获取当前登录用户的信息

**认证**: 需要 JWT Token

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "id": "uuid",
    "username": "john_doe",
    "email": "john@example.com",
    "avatar": "url",
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

### 3.2 更新用户信息

**PUT** `/api/v1/users/me`

**描述**: 更新当前用户的信息

**认证**: 需要 JWT Token

**请求体**:
```json
{
  "username": "new_username",
  "avatar": "base64_or_url"
}
```

**响应**:
```json
{
  "code": 200,
  "message": "更新成功",
  "data": {
    "id": "uuid",
    "username": "new_username",
    "email": "john@example.com",
    "avatar": "url",
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

**错误响应**:
- 409: 用户名已存在

### 3.3 修改密码

**PUT** `/api/v1/users/me/password`

**描述**: 修改当前用户密码

**认证**: 需要 JWT Token

**请求体**:
```json
{
  "old_password": "OldPassword123!",
  "new_password": "NewPassword456!"
}
```

**响应**:
```json
{
  "code": 200,
  "message": "密码修改成功"
}
```

**错误响应**:
- 400: 原密码错误
- 422: 新密码不符合要求

### 3.4 获取用户绘本列表

**GET** `/api/v1/users/me/books`

**描述**: 获取当前用户的所有绘本

**认证**: 需要 JWT Token

**查询参数**:
- `page`: 页码（默认 1）
- `limit`: 每页数量（默认 20）
- `status`: 状态筛选（draft/published/archived）

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "items": [
      {
        "id": "uuid",
        "title": "我的绘本",
        "description": "绘本描述",
        "cover_image": "url",
        "status": "draft",
        "page_count": 5,
        "created_at": "2026-09-17T10:00:00Z",
        "updated_at": "2026-09-17T12:00:00Z"
      }
    ],
    "total": 10,
    "page": 1,
    "limit": 20
  }
}
```

---

## 4. 绘本模块 (Book)

### 4.1 创建绘本

**POST** `/api/v1/books`

**描述**: 创建新的绘本项目

**认证**: 需要 JWT Token

**请求体**:
```json
{
  "title": "小红帽的故事",
  "description": "一个关于勇敢小女孩的故事"
}
```

**响应**:
```json
{
  "code": 201,
  "message": "创建成功",
  "data": {
    "id": "uuid",
    "title": "小红帽的故事",
    "description": "一个关于勇敢小女孩的故事",
    "cover_image": null,
    "status": "draft",
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

### 4.2 获取绘本详情

**GET** `/api/v1/books/{book_id}`

**描述**: 获取指定绘本的详细信息

**认证**: 需要 JWT Token

**路径参数**:
- `book_id`: 绘本 ID

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "id": "uuid",
    "title": "小红帽的故事",
    "description": "一个关于勇敢小女孩的故事",
    "cover_image": "url",
    "status": "draft",
    "pages": [
      {
        "id": "page-uuid",
        "page_number": 1,
        "title": "第一页",
        "thumbnail": "url"
      }
    ],
    "created_at": "2026-09-17T10:00:00Z",
    "updated_at": "2026-09-17T12:00:00Z"
  }
}
```

**错误响应**:
- 404: 绘本不存在
- 403: 无权访问该绘本

### 4.3 更新绘本

**PUT** `/api/v1/books/{book_id}`

**描述**: 更新绘本信息

**认证**: 需要 JWT Token

**路径参数**:
- `book_id`: 绘本 ID

**请求体**:
```json
{
  "title": "新标题",
  "description": "新描述",
  "cover_image": "url",
  "status": "published"
}
```

**响应**:
```json
{
  "code": 200,
  "message": "更新成功",
  "data": {
    "id": "uuid",
    "title": "新标题",
    "description": "新描述",
    "cover_image": "url",
    "status": "published",
    "updated_at": "2026-09-17T13:00:00Z"
  }
}
```

### 4.4 删除绘本

**DELETE** `/api/v1/books/{book_id}`

**描述**: 删除指定绘本

**认证**: 需要 JWT Token

**路径参数**:
- `book_id`: 绘本 ID

**响应**:
```json
{
  "code": 200,
  "message": "删除成功"
}
```

**错误响应**:
- 404: 绘本不存在
- 403: 无权删除该绘本

---

> **已移除**：原 §5 页面模块、§6 页面元素模块（在线编辑子系统遗留），`/pages`、`/elements` 接口不再提供。

## 7. 角色模块 (Character)（第二阶段）

### 7.1 创建角色

**POST** `/api/v1/characters`

**描述**: 创建新角色

**认证**: 需要 JWT Token

**请求体**:
```json
{
  "name": "小明",
  "description": "一个勇敢的小男孩",
  "age": 8,
  "appearance": "黑色短发，黄色雨衣，蓝色裤子",
  "style_info": {
    "artStyle": "cartoon",
    "colorPalette": ["#FFD700", "#4169E1"]
  }
}
```

**响应**:
```json
{
  "code": 201,
  "message": "创建成功",
  "data": {
    "id": "character-uuid",
    "name": "小明",
    "description": "一个勇敢的小男孩",
    "age": 8,
    "appearance": "黑色短发，黄色雨衣，蓝色裤子",
    "style_info": {},
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

### 7.2 获取角色列表

**GET** `/api/v1/characters`

**描述**: 获取当前用户的所有角色

**认证**: 需要 JWT Token

**查询参数**:
- `page`: 页码（默认 1）
- `limit`: 每页数量（默认 20）

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "items": [
      {
        "id": "character-uuid",
        "name": "小明",
        "description": "一个勇敢的小男孩",
        "age": 8,
        "appearance": "黑色短发，黄色雨衣，蓝色裤子",
        "style_info": {},
        "created_at": "2026-09-17T10:00:00Z"
      }
    ],
    "total": 5,
    "page": 1,
    "limit": 20
  }
}
```

### 7.3 获取角色详情

**GET** `/api/v1/characters/{character_id}`

**描述**: 获取指定角色的详细信息

**认证**: 需要 JWT Token

**路径参数**:
- `character_id`: 角色 ID

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "id": "character-uuid",
    "name": "小明",
    "description": "一个勇敢的小男孩",
    "age": 8,
    "appearance": "黑色短发，黄色雨衣，蓝色裤子",
    "style_info": {},
    "created_at": "2026-09-17T10:00:00Z",
    "updated_at": "2026-09-17T12:00:00Z"
  }
}
```

### 7.4 更新角色

**PUT** `/api/v1/characters/{character_id}`

**描述**: 更新角色信息

**认证**: 需要 JWT Token

**路径参数**:
- `character_id`: 角色 ID

**请求体**:
```json
{
  "name": "小明",
  "description": "更新后的描述",
  "appearance": "更新后的外观描述"
}
```

**响应**:
```json
{
  "code": 200,
  "message": "更新成功",
  "data": {
    "id": "character-uuid",
    "name": "小明",
    "description": "更新后的描述",
    "appearance": "更新后的外观描述",
    "updated_at": "2026-09-17T13:00:00Z"
  }
}
```

### 7.5 删除角色

**DELETE** `/api/v1/characters/{character_id}`

**描述**: 删除指定角色

**认证**: 需要 JWT Token

**路径参数**:
- `character_id`: 角色 ID

**响应**:
```json
{
  "code": 200,
  "message": "删除成功"
}
```

---

## 8. AI 生成模块 (Generation)

### 8.1 创建生成任务

**POST** `/api/v1/generation`

**描述**: 创建图像生成任务

**认证**: 需要 JWT Token

**请求体**:
```json
{
  "book_id": "book-uuid",
  "page_id": "page-uuid",
  "text": "小女孩在雨后的森林里发现了一只受伤的小鸟。",
  "character_ids": ["character-1"],
  "style": "watercolor",
  "parameters": {
    "size": "1024x1024",
    "quality": "standard"
  }
}
```

**响应**:
```json
{
  "code": 201,
  "message": "任务创建成功",
  "data": {
    "task_id": "task-uuid",
    "status": "pending",
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

### 8.2 获取任务状态

**GET** `/api/v1/generation/{task_id}`

**描述**: 获取生成任务的状态

**认证**: 需要 JWT Token

**路径参数**:
- `task_id`: 任务 ID

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "task_id": "task-uuid",
    "status": "completed",
    "prompt": "生成使用的提示词",
    "result": {
      "image_url": "url",
      "width": 1024,
      "height": 1024
    },
    "created_at": "2026-09-17T10:00:00Z",
    "completed_at": "2026-09-17T10:01:30Z"
  }
}
```

**任务状态枚举**:
- `pending`: 待处理
- `processing`: 处理中
- `completed`: 已完成
- `failed`: 失败

### 8.3 获取用户生成历史

**GET** `/api/v1/generation/history`

**描述**: 获取当前用户的生成历史

**认证**: 需要 JWT Token

**查询参数**:
- `page`: 页码（默认 1）
- `limit`: 每页数量（默认 20）
- `book_id`: 按绘本筛选

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "items": [
      {
        "task_id": "task-uuid",
        "status": "completed",
        "prompt": "生成使用的提示词",
        "original_text": "用户输入的原始文本",
        "result": {
          "image_url": "url",
          "width": 1024,
          "height": 1024
        },
        "created_at": "2026-09-17T10:00:00Z",
        "completed_at": "2026-09-17T10:01:30Z"
      }
    ],
    "total": 50,
    "page": 1,
    "limit": 20
  }
}
```

### 8.4 取消任务

**POST** `/api/v1/generation/{task_id}/cancel`

**描述**: 取消正在进行的生成任务

**认证**: 需要 JWT Token

**路径参数**:
- `task_id`: 任务 ID

**响应**:
```json
{
  "code": 200,
  "message": "任务已取消"
}
```

### 8.5 重新生成

**POST** `/api/v1/generation/{task_id}/regenerate`

**描述**: 基于已有任务重新生成

**认证**: 需要 JWT Token

**路径参数**:
- `task_id`: 原任务 ID

**请求体**:
```json
{
  "parameters": {
    "seed": 12345
  }
}
```

**响应**:
```json
{
  "code": 201,
  "message": "重新生成任务创建成功",
  "data": {
    "task_id": "new-task-uuid",
    "status": "pending",
    "created_at": "2026-09-17T10:05:00Z"
  }
}
```

---

## 9. 图片模块 (Image)

### 9.1 下载生成图片（加分项）

**GET** `/api/v1/images/{image_id}/download`

**描述**: 下载单张 AI 生成图片

**认证**: 需要 JWT Token

**路径参数**:
- `image_id`: 图片 ID

**响应**: 二进制图片流（`Content-Disposition: attachment`）

**错误响应**:
- 403: 无权访问该图片
- 404: 图片不存在

> **已移除**：原「上传图片」接口（`POST /images/upload`）属在线编辑子系统，本轮不再提供上传入口。

### 9.2 获取图片信息

**GET** `/api/v1/images/{image_id}`

**描述**: 获取指定图片的信息

**认证**: 需要 JWT Token

**路径参数**:
- `image_id`: 图片 ID

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "id": "image-uuid",
    "file_path": "url",
    "file_size": 1024000,
    "width": 800,
    "height": 600,
    "format": "jpeg",
    "created_at": "2026-09-17T10:00:00Z"
  }
}
```

### 9.3 删除图片

**DELETE** `/api/v1/images/{image_id}`

**描述**: 删除指定图片

**认证**: 需要 JWT Token

**路径参数**:
- `image_id`: 图片 ID

**响应**:
```json
{
  "code": 200,
  "message": "删除成功"
}
```

### 9.4 获取用户图片列表

**GET** `/api/v1/images`

**描述**: 获取当前用户的所有图片

**认证**: 需要 JWT Token

**查询参数**:
- `page`: 页码（默认 1）
- `limit`: 每页数量（默认 20）
- `book_id`: 按绘本筛选

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "items": [
      {
        "id": "image-uuid",
        "file_path": "url",
        "file_size": 1024000,
        "width": 800,
        "height": 600,
        "format": "jpeg",
        "created_at": "2026-09-17T10:00:00Z"
      }
    ],
    "total": 100,
    "page": 1,
    "limit": 20
  }
}
```

---

## 11. 健康检查模块 (Health)

### 11.1 系统健康检查

**GET** `/api/v1/health`

**描述**: 检查系统各组件状态

**认证**: 不需要

**响应**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "status": "healthy",
    "timestamp": "2026-09-17T10:00:00Z",
    "services": {
      "database": "healthy",
      "redis": "healthy",
      "ai_service": "healthy"
    }
  }
}
```

---

## 12. API 安全要求

### 12.1 认证要求

- 所有需要认证的 API 必须在 Header 中携带 JWT Token
- Token 格式: `Authorization: Bearer <token>`

### 12.2 权限控制

- 用户只能访问自己的资源
- API 自动验证资源所有权

### 12.3 速率限制

- 未认证 API: 100 次/分钟
- 认证 API: 1000 次/分钟
- AI 生成 API: 10 次/分钟

### 12.4 请求大小限制

- 普通 API: 1MB
- JSON 请求: 1MB

> 原「文件上传 10MB」限制随图片上传接口一并移除。

### 12.5 凭据与加密（新增）

- 密码字段仅接收明文输入，服务端 **bcrypt 加盐哈希**后存储；任何接口都不得返回密码或哈希
- 登录/注册请求体中的 `password` 不写入日志
- 登录失败统一返回「邮箱或密码错误」，不区分账号是否存在
- Token 通过 `Authorization` 头传输，不放在 URL 查询参数中
- 生产环境强制 HTTPS

---

**文档结束**
