#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

//字段校验错误
struct FieldError {
    std::string field;
    std::string message;
};

//注册请求
struct RegisterRequest {
    std::string username;
    std::string email;
    std::string password;
    std::string guestId;  // 可选：游客身份，注册时认领其生成结果

    static RegisterRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

//登录请求
struct LoginRequest {
    std::string email;
    std::string password;
    std::string guestId;  // 可选：游客身份，登录时认领其生成结果

    static LoginRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

//游客会话（Guest-first：创建影子游客并签发游客 Token）
struct GuestSession {
    std::string guestId;
    std::string guestToken;
    int quota = 1;  // 可免费生成的图片数（用于前端提示）

    json toJson() const;
};

//刷新 Token 请求
struct RefreshRequest {
    std::string refreshToken;

    static RefreshRequest fromJson(const json &j);
    bool validate() const;
};

//用户信息（嵌套在 AuthResponse 中)
struct UserInfo {
    std::string id;
    std::string username;
    std::string email;
    std::string avatar;
    std::string createdAt;

    json toJson() const;
};

//认证响应
struct AuthResponse {
    UserInfo user;
    std::string accessToken;
    std::string refreshToken;

    json toJson() const;
};