#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "AuthDto.h"

using json = nlohmann::json;

//更新用户信息请求
struct UpdateUserRequest{
    std::string username;
    std::string avatar;

    static UpdateUserRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

//密码修改请求
struct ChangePasswordRequest {
    std::string oldPassword;
    std::string newPassword;

    static ChangePasswordRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

//用户信息响应
struct UserResponse {
    std::string id;
    std::string username;
    std::string email;
    std::string avatar;
    std::string createdAt;

    json toJson() const;
};
