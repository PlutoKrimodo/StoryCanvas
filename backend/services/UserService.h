#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include "../dto/AuthDto.h"
#include "../dto/UserDto.h"

using json = nlohmann::json;

struct ServiceResult{
    bool success = false;
    int code = 500;
    std::string message;
    json data;
};

class UserService{
public:
    //注册
    static ServiceResult registerUser(const RegisterRequest& req);
    //登录
    static ServiceResult loginUser(const LoginRequest& req);
    //刷新token
    static ServiceResult refreshToken(const std::string& refreshToken);
    //登出
    static void logout(const std::string &accessToken, const std::string &refreshToken);
    //获取用户信息
    static ServiceResult getUserById(const std::string &userId);
    //更新用户资料
    static ServiceResult updateUser(const std::string &userId, const UpdateUserRequest& req);
    //修改密码
    static ServiceResult changePassword(const std::string &userId, const ChangePasswordRequest& req);
};