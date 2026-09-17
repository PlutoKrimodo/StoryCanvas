# C++/Drogon 项目结构设计

## 基于生成式 AI 的儿童绘本智能创作与在线编辑系统

**版本**: 1.0  
**日期**: 2026-09-17  
**作者**: StoryCanvas 开发团队

---

## 1. 项目目录结构

### 1.1 完整目录结构

```
backend/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 项目说明
├── config/                     # 配置文件目录
│   ├── config.json             # 主配置文件
│   ├── config.json.example     # 配置示例
│   └── logging.json            # 日志配置
├── controllers/                # 控制器层
│   ├── AuthController.h
│   ├── AuthController.cc
│   ├── UserController.h
│   ├── UserController.cc
│   ├── BookController.h
│   ├── BookController.cc
│   ├── PageController.h
│   ├── PageController.cc
│   ├── ElementController.h
│   ├── ElementController.cc
│   ├── CharacterController.h
│   ├── CharacterController.cc
│   ├── ImageController.h
│   ├── ImageController.cc
│   ├── GenerationController.h
│   ├── GenerationController.cc
│   ├── ExportController.h
│   ├── ExportController.cc
│   └── HealthController.h
│   └── HealthController.cc
├── services/                   # 业务逻辑层
│   ├── UserService.h
│   ├── UserService.cc
│   ├── BookService.h
│   ├── BookService.cc
│   ├── PageService.h
│   ├── PageService.cc
│   ├── ElementService.h
│   ├── ElementService.cc
│   ├── CharacterService.h
│   ├── CharacterService.cc
│   ├── ImageService.h
│   ├── ImageService.cc
│   ├── GenerationService.h
│   ├── GenerationService.cc
│   ├── ExportService.h
│   ├── ExportService.cc
│   └── AiClient.h              # AI Service 客户端
│   └── AiClient.cc
├── models/                     # 数据模型层
│   ├── User.h
│   ├── User.cc
│   ├── Book.h
│   ├── Book.cc
│   ├── Page.h
│   ├── Page.cc
│   ├── PageElement.h
│   ├── PageElement.cc
│   ├── Character.h
│   ├── Character.cc
│   ├── GenerationTask.h
│   ├── GenerationTask.cc
│   ├── GeneratedImage.h
│   └── GeneratedImage.cc
├── repositories/               # 数据访问层
│   ├── UserRepository.h
│   ├── UserRepository.cc
│   ├── BookRepository.h
│   ├── BookRepository.cc
│   ├── PageRepository.h
│   ├── PageRepository.cc
│   ├── PageElementRepository.h
│   ├── PageElementRepository.cc
│   ├── CharacterRepository.h
│   ├── CharacterRepository.cc
│   ├── GenerationTaskRepository.h
│   ├── GenerationTaskRepository.cc
│   ├── GeneratedImageRepository.h
│   └── GeneratedImageRepository.cc
├── filters/                    # 过滤器/中间件
│   ├── JwtFilter.h
│   ├── JwtFilter.cc
│   ├── CorsFilter.h
│   ├── CorsFilter.cc
│   ├── RateLimitFilter.h
│   └── RateLimitFilter.cc
├── dto/                        # 数据传输对象
│   ├── AuthDto.h
│   ├── UserDto.h
│   ├── BookDto.h
│   ├── PageDto.h
│   ├── ElementDto.h
│   ├── CharacterDto.h
│   ├── GenerationDto.h
│   ├── ImageDto.h
│   ├── ExportDto.h
│   └── ApiResponse.h
├── utils/                      # 工具类
│   ├── JwtUtils.h
│   ├── JwtUtils.cc
│   ├── PasswordUtils.h
│   ├── PasswordUtils.cc
│   ├── FileUtils.h
│   ├── FileUtils.cc
│   ├── Validator.h
│   ├── Validator.cc
│   ├── ConfigManager.h
│   └── ConfigManager.cc
├── sql/                        # 数据库脚本
│   ├── init.sql                # 初始化脚本
│   ├── migrations/             # 迁移脚本
│   └── seeds/                  # 测试数据
├── tests/                      # 测试
│   ├── CMakeLists.txt
│   ├── test_auth_controller.cc
│   ├── test_user_service.cc
│   └── ...
├── build/                      # 构建目录
└── main.cc                     # 程序入口
```

---

## 2. 分层架构说明

### 2.1 Controller 层（控制器）

**职责**:
- 接收 HTTP 请求
- 解析请求参数
- 调用 Service 层
- 构造响应
- 输入验证

**特点**:
- 不包含业务逻辑
- 只负责请求/响应处理
- 调用 Service 层完成业务

**示例代码**:
```cpp
// controllers/AuthController.h
#pragma once
#include <drogon/HttpSimpleController.h>

class AuthController : public drogon::HttpSimpleController<AuthController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback
    ) override;
    
    static void init();
    static void shutdown();
    
    // API 方法
    void registerUser(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);
    void loginUser(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
    void refreshToken(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);
    void logout(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
};

// controllers/AuthController.cc
#include "AuthController.h"
#include "../services/UserService.h"
#include "../dto/AuthDto.h"
#include "../utils/JwtUtils.h"

void AuthController::registerUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    
    // 1. 解析请求
    auto json = req->getJsonObject();
    if (!json) {
        callback(HttpResponse::newHttpResponse(k400BadRequest, CT_APPLICATION_JSON));
        return;
    }
    
    // 2. 验证输入
    RegisterRequest request;
    if (!request.fromJson(json)) {
        callback(HttpResponse::newHttpResponse(k422UnprocessableEntity, CT_APPLICATION_JSON));
        return;
    }
    
    // 3. 调用 Service
    auto userService = std::make_shared<UserService>();
    auto result = userService->registerUser(request);
    
    // 4. 构造响应
    if (result.success) {
        auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
        resp->setStatusCode(k201Created);
        callback(resp);
    } else {
        auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }
}
```

### 2.2 Service 层（业务逻辑）

**职责**:
- 实现业务逻辑
- 事务管理
- 调用 Repository 层
- 调用外部服务

**特点**:
- 包含核心业务逻辑
- 可被多个 Controller 调用
- 处理业务规则

**示例代码**:
```cpp
// services/UserService.h
#pragma once
#include "../models/User.h"
#include "../dto/AuthDto.h"
#include "../repositories/UserRepository.h"

class UserService {
public:
    UserService();
    ~UserService() = default;
    
    // 用户注册
    ServiceResult registerUser(const RegisterRequest &request);
    
    // 用户登录
    ServiceResult loginUser(const LoginRequest &request);
    
    // 获取用户信息
    ServiceResult getUserById(const std::string &userId);
    
    // 更新用户信息
    ServiceResult updateUser(const std::string &userId, 
                            const UpdateUserRequest &request);
    
    // 修改密码
    ServiceResult changePassword(const std::string &userId,
                                 const ChangePasswordRequest &request);

private:
    std::shared_ptr<UserRepository> userRepository_;
};

// services/UserService.cc
#include "UserService.h"
#include "../utils/PasswordUtils.h"
#include "../utils/JwtUtils.h"

UserService::UserService() 
    : userRepository_(std::make_shared<UserRepository>()) {}

ServiceResult UserService::registerUser(const RegisterRequest &request) {
    ServiceResult result;
    
    // 1. 检查用户名是否已存在
    if (userRepository_->findByUsername(request.username)) {
        result.success = false;
        result.message = "用户名已存在";
        result.code = 409;
        return result;
    }
    
    // 2. 检查邮箱是否已存在
    if (userRepository_->findByEmail(request.email)) {
        result.success = false;
        result.message = "邮箱已存在";
        result.code = 409;
        return result;
    }
    
    // 3. 创建用户
    User user;
    user.id = generateUuid();
    user.username = request.username;
    user.email = request.email;
    user.passwordHash = PasswordUtils::hashPassword(request.password);
    user.createdAt = getCurrentTimestamp();
    user.updatedAt = getCurrentTimestamp();
    
    // 4. 保存到数据库
    if (!userRepository_->create(user)) {
        result.success = false;
        result.message = "创建用户失败";
        result.code = 500;
        return result;
    }
    
    // 5. 生成 JWT Token
    auto tokenPair = JwtUtils::generateTokenPair(user.id, user.username);
    
    // 6. 构造结果
    result.success = true;
    result.message = "注册成功";
    result.code = 201;
    result.data = {
        {"user", user.toJson()},
        {"access_token", tokenPair.accessToken},
        {"refresh_token", tokenPair.refreshToken}
    };
    
    return result;
}
```

### 2.3 Repository 层（数据访问）

**职责**:
- 数据库 CRUD 操作
- SQL 查询
- 数据映射

**特点**:
- 封装数据库操作
- 提供数据访问接口
- 使用 ORM 简化操作

**示例代码**:
```cpp
// repositories/UserRepository.h
#pragma once
#include "../models/User.h"
#include <drogon/orm/Criteria.h>

class UserRepository {
public:
    UserRepository();
    ~UserRepository() = default;
    
    // 创建用户
    bool create(const User &user);
    
    // 根据 ID 查找
    std::optional<User> findById(const std::string &id);
    
    // 根据用户名查找
    std::optional<User> findByUsername(const std::string &username);
    
    // 根据邮箱查找
    std::optional<User> findByEmail(const std::string &email);
    
    // 更新用户
    bool update(const User &user);
    
    // 删除用户
    bool deleteById(const std::string &id);
    
    // 分页查询
    std::pair<std::vector<User>, int> findAll(int page, int limit);

private:
    std::shared_ptr<drogon::orm::DbClient> dbClient_;
};

// repositories/UserRepository.cc
#include "UserRepository.h"
#include <drogon/drogon.h>

UserRepository::UserRepository() 
    : dbClient_(drogon::app().getDbClient()) {}

std::optional<User> UserRepository::findByUsername(const std::string &username) {
    auto sql = "SELECT * FROM users WHERE username = $1";
    auto result = dbClient_->execSqlSync(sql, username);
    
    if (result.empty()) {
        return std::nullopt;
    }
    
    User user;
    user.fromJson(result[0]);
    return user;
}

bool UserRepository::create(const User &user) {
    auto sql = R"(
        INSERT INTO users (id, username, email, password_hash, avatar, is_active, created_at, updated_at)
        VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
    )";
    
    try {
        dbClient_->execSqlSync(sql, 
            user.id, 
            user.username, 
            user.email, 
            user.passwordHash,
            user.avatar.value_or(""),
            user.isActive,
            user.createdAt,
            user.updatedAt
        );
        return true;
    } catch (const std::exception &e) {
        LOG_ERROR << "创建用户失败: " << e.what();
        return false;
    }
}
```

### 2.4 Model 层（数据模型）

**职责**:
- 定义数据结构
- ORM 映射
- 数据验证

**特点**:
- 纯数据对象
- 包含序列化/反序列化
- 包含字段验证

**示例代码**:
```cpp
// models/User.h
#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class User {
public:
    // 字段
    std::string id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::optional<std::string> avatar;
    bool isActive = true;
    std::string createdAt;
    std::string updatedAt;
    
    // 序列化
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json &json);
    
    // 验证
    bool validate() const;
    std::vector<std::string> getErrors() const;
};

// models/User.cc
#include "User.h"
#include "../utils/Validator.h"

nlohmann::json User::toJson() const {
    return {
        {"id", id},
        {"username", username},
        {"email", email},
        {"avatar", avatar.value_or(nullptr)},
        {"is_active", isActive},
        {"created_at", createdAt},
        {"updated_at", updatedAt}
    };
}

void User::fromJson(const nlohmann::json &json) {
    id = json["id"];
    username = json["username"];
    email = json["email"];
    if (json.contains("avatar") && !json["avatar"].is_null()) {
        avatar = json["avatar"];
    }
    isActive = json.value("is_active", true);
    createdAt = json["created_at"];
    updatedAt = json["updated_at"];
}

bool User::validate() const {
    return !username.empty() && 
           !email.empty() && 
           Validator::isValidEmail(email);
}

std::vector<std::string> User::getErrors() const {
    std::vector<std::string> errors;
    if (username.empty()) errors.push_back("用户名不能为空");
    if (email.empty()) errors.push_back("邮箱不能为空");
    if (!Validator::isValidEmail(email)) errors.push_back("邮箱格式无效");
    return errors;
}
```

### 2.5 Filter 层（中间件）

**职责**:
- 请求过滤
- JWT 验证
- CORS 处理
- 速率限制

**特点**:
- 在请求处理前执行
- 可修改请求/响应
- 可中断请求

**示例代码**:
```cpp
// filters/JwtFilter.h
#pragma once
#include <drogon/HttpFilter.h>

class JwtFilter : public drogon::HttpFilter<JwtFilter> {
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};

// filters/JwtFilter.cc
#include "JwtFilter.h"
#include "../utils/JwtUtils.h"

void JwtFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb) {
    // 1. 获取 Token
    auto token = req->getHeader("Authorization");
    if (token.empty() || token.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse(k401Unauthorized, CT_APPLICATION_JSON);
        resp->setBody("{\"code\":401,\"message\":\"未提供有效的认证令牌\"}");
        fcb(resp);
        return;
    }
    
    // 2. 验证 Token
    auto jwt = token.substr(7);
    auto payload = JwtUtils::verifyToken(jwt);
    if (!payload) {
        auto resp = HttpResponse::newHttpResponse(k401Unauthorized, CT_APPLICATION_JSON);
        resp->setBody("{\"code\":401,\"message\":\"认证令牌无效或已过期\"}");
        fcb(resp);
        return;
    }
    
    // 3. 将用户信息添加到请求属性
    req->setAttributes("user_id", payload->userId);
    req->setAttributes("username", payload->username);
    
    // 4. 继续处理请求
    fccb();
}
```

### 2.6 DTO 层（数据传输对象）

**职责**:
- 定义请求/响应数据结构
- 数据验证
- 序列化/反序列化

**特点**:
- 不包含业务逻辑
- 用于层间数据传递
- 包含验证规则

**示例代码**:
```cpp
// dto/AuthDto.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct RegisterRequest {
    std::string username;
    std::string email;
    std::string password;
    
    bool fromJson(const nlohmann::json &json);
    nlohmann::json toJson() const;
    bool validate() const;
    std::vector<std::string> getErrors() const;
};

struct LoginRequest {
    std::string email;
    std::string password;
    
    bool fromJson(const nlohmann::json &json);
    nlohmann::json toJson() const;
    bool validate() const;
};

struct AuthResponse {
    struct UserInfo {
        std::string id;
        std::string username;
        std::string email;
        std::string avatar;
        std::string createdAt;
        
        nlohmann::json toJson() const;
    };
    
    UserInfo user;
    std::string accessToken;
    std::string refreshToken;
    
    nlohmann::json toJson() const;
};

// dto/AuthDto.cc
#include "AuthDto.h"
#include "../utils/Validator.h"

bool RegisterRequest::fromJson(const nlohmann::json &json) {
    try {
        username = json.at("username");
        email = json.at("email");
        password = json.at("password");
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

bool RegisterRequest::validate() const {
    return !username.empty() && 
           !email.empty() && 
           !password.empty() &&
           password.length() >= 8 &&
           Validator::isValidEmail(email);
}

std::vector<std::string> RegisterRequest::getErrors() const {
    std::vector<std::string> errors;
    if (username.empty()) errors.push_back("用户名不能为空");
    if (email.empty()) errors.push_back("邮箱不能为空");
    if (!Validator::isValidEmail(email)) errors.push_back("邮箱格式无效");
    if (password.empty()) errors.push_back("密码不能为空");
    if (password.length() < 8) errors.push_back("密码长度不能少于8位");
    return errors;
}

nlohmann::json AuthResponse::toJson() const {
    return {
        {"user", user.toJson()},
        {"access_token", accessToken},
        {"refresh_token", refreshToken}
    };
}
```

### 2.7 Utils 层（工具类）

**职责**:
- 提供通用工具函数
- JWT 处理
- 密码加密
- 文件操作
- 配置管理

**特点**:
- 无状态
- 可复用
- 独立于业务逻辑

**示例代码**:
```cpp
// utils/JwtUtils.h
#pragma once
#include <string>
#include <optional>

struct TokenPayload {
    std::string userId;
    std::string username;
    std::string exp;
};

struct TokenPair {
    std::string accessToken;
    std::string refreshToken;
};

class JwtUtils {
public:
    static TokenPair generateTokenPair(const std::string &userId, 
                                       const std::string &username);
    static std::optional<TokenPayload> verifyToken(const std::string &token);
    static std::string generateRefreshToken();

private:
    static std::string secret_;
    static int accessTokenExpiry_;
    static int refreshTokenExpiry_;
};

// utils/PasswordUtils.h
#pragma once
#include <string>

class PasswordUtils {
public:
    static std::string hashPassword(const std::string &password);
    static bool verifyPassword(const std::string &password, 
                               const std::string &hash);
    static std::string generateSalt();
};

// utils/Validator.h
#pragma once
#include <string>

class Validator {
public:
    static bool isValidEmail(const std::string &email);
    static bool isValidUsername(const std::string &username);
    static bool isValidPassword(const std::string &password);
    static bool isValidUuid(const std::string &uuid);
    static bool isValidUrl(const std::string &url);
};
```

---

## 3. CMakeLists.txt 配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(storycanvas_backend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(PostgreSQL REQUIRED)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DROGON_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
)

# 收集源文件
file(GLOB_RECURSE SOURCES 
    "controllers/*.cc"
    "services/*.cc"
    "repositories/*.cc"
    "models/*.cc"
    "filters/*.cc"
    "dto/*.cc"
    "utils/*.cc"
)

# 创建可执行文件
add_executable(${PROJECT_NAME} main.cc ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    ${DROGON_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pq
    pthread
)

# 安装配置
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES config/config.json
    DESTINATION etc
)
```

---

## 4. 配置文件

### 4.1 config.json

```json
{
  "app": {
    "name": "StoryCanvas Backend",
    "port": 8080,
    "threads": 4,
    "document_root": "./static",
    "home_page": "index.html"
  },
  "listeners": [
    {
      "address": "0.0.0.0",
      "port": 8080,
      "filters": ["JwtFilter"]
    }
  ],
  "database": {
    "type": "postgresql",
    "host": "localhost",
    "port": 5432,
    "database": "storycanvas",
    "username": "postgres",
    "password": "${DB_PASSWORD}",
    "min_connections": 5,
    "max_connections": 20
  },
  "redis": {
    "host": "localhost",
    "port": 6379,
    "password": "${REDIS_PASSWORD}",
    "database": 0
  },
  "jwt": {
    "secret": "${JWT_SECRET}",
    "access_token_expiry": 3600,
    "refresh_token_expiry": 604800
  },
  "ai_service": {
    "url": "http://localhost:8000",
    "api_key": "${AI_SERVICE_API_KEY}",
    "timeout": 60
  },
  "storage": {
    "type": "local",
    "local_path": "./storage",
    "max_upload_size": 10485760
  },
  "log": {
    "loggers": [
      {
        "name": "basic",
        "type": "console",
        "level": "debug"
      },
      {
        "name": "file",
        "type": "file",
        "level": "info",
        "path": "./logs",
        "filename_pattern": "%Y-%m-%d.log"
      }
    ]
  }
}
```

---

## 5. 程序入口

### 5.1 main.cc

```cpp
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include "utils/ConfigManager.h"

int main() {
    // 初始化配置
    ConfigManager::init();
    
    // 配置 Drogon
    auto &app = drogon::app();
    
    // 加载配置文件
    app.loadConfigFile("./config/config.json");
    
    // 注册路由
    // ... 路由注册代码
    
    // 启动应用
    spdlog::info("StoryCanvas Backend 启动中...");
    app.run();
    
    return 0;
}
```

---

## 6. 设计原则

### 6.1 单一职责
每个类只负责一个明确的功能。

### 6.2 依赖注入
通过构造函数注入依赖，便于测试和维护。

### 6.3 接口隔离
定义清晰的接口，降低耦合度。

### 6.4 开闭原则
对扩展开放，对修改关闭。

---

## 7. 避免的问题

### 7.1 避免 Controller 包含业务逻辑
- Controller 只负责请求/响应处理
- 业务逻辑放在 Service 层

### 7.2 避免 Repository 包含业务逻辑
- Repository 只负责数据访问
- 不包含业务规则

### 7.3 避免循环依赖
- 使用接口解耦
- 控制依赖方向

---

**文档结束**
