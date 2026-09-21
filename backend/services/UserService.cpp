#include "UserService.h"
#include "../repositories/UserRepository.h"
#include "../utils/JwtUtils.h"
#include "../utils/PasswordUtils.h"
#include "spdlog/spdlog.h"

ServiceResult UserService::registerUser(const RegisterRequest &req) {
    ServiceResult result;

    if(UserRepository::existsByUsername(req.username)) {
        result.code = 409;
        result.message = "用户名已存在";
        return result;
    }
    if(UserRepository::existsByEmail(req.email)) {
        result.code = 409;
        result.message = "邮箱已存在";
        return result;
    }
    
    //创建用户
    User user;
    user.username = req.username;
    user.email = req.email;
    user.passwordHash = PasswordUtils::hashPassword(req.password);

    if(!UserRepository::create(user)) {
        result.code = 500;
        result.message = "用户注册失败";
        return result;
    }

    auto tokenPair = JwtUtils::generateTokenPair(user.id, user.username);

    AuthResponse authResponse;  
    authResponse.user.id = user.id;
    authResponse.user.username = user.username;
    authResponse.user.email = user.email;
    authResponse.user.createdAt = user.createdAt;
    authResponse.accessToken = tokenPair.accessToken;
    authResponse.refreshToken = tokenPair.refreshToken;

    result.success = true;
    result.code = 201;
    result.message = "注册成功";
    result.data = authResponse.toJson();
    return result;
}

ServiceResult UserService::loginUser(const LoginRequest &req) {
    ServiceResult result;

    auto userOpt = UserRepository::findByEmail(req.email);
    if (!userOpt) {
        result.code = 401;
        result.message = "邮箱或密码有错误";
        return result;
    }

    auto &user = *userOpt;

    if (!PasswordUtils::verifyPassword(req.password, user.passwordHash)) {
        result.code = 401;
        result.message = "邮箱或密码有错误";
        return result;
    }

    auto tokenPair = JwtUtils::generateTokenPair(user.id, user.username);

    AuthResponse authResponse;
    authResponse.user.id = user.id;
    authResponse.user.username = user.username;
    authResponse.user.email = user.email;
    authResponse.user.createdAt = user.createdAt;
    authResponse.accessToken = tokenPair.accessToken;
    authResponse.refreshToken = tokenPair.refreshToken;

    result.success = true;
    result.code = 200;
    result.message = "登录成功";
    result.data = authResponse.toJson();
    return result;
}

ServiceResult UserService::refreshToken(const std::string &refreshToken) {
    ServiceResult result;

    auto payloadOpt = JwtUtils::verifyToken(refreshToken, "refresh");
    if (!payloadOpt) {
        result.code = 401;
        result.message = "Refresh Token 无效或已过期";
        return result;
    }

    if (JwtUtils::isRevoked(payloadOpt->jti)) {
        result.code = 401;
        result.message = "Refresh Token 已被撤销";
        return result;
    }

    auto refreshResult = JwtUtils::refresh(refreshToken);
    if (!refreshResult) {
        result.code = 401;
        result.message = "刷新失败";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "刷新成功";
    result.data = {
        {"access_token", refreshResult->accessToken},
        {"refresh_token", refreshResult->refreshToken}
    };
    return result;
}

void UserService::logout(const std::string &accessToken, const std::string &refreshToken) {
    if (!accessToken.empty()) {
        JwtUtils::revokeToken(accessToken);
    }
    if (!refreshToken.empty()) {
        JwtUtils::revokeToken(refreshToken);
    }
}

ServiceResult UserService::getUserById(const std::string &userId) {
    ServiceResult result;

    auto userOpt = UserRepository::findById(userId);
    if (!userOpt) {
        result.code = 404;
        result.message = "用户不存在";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = userOpt->toJson();
    return result;
}

ServiceResult UserService::updateUser(const std::string &userId, const UpdateUserRequest &req) {
    ServiceResult result;

    auto userOpt = UserRepository::findById(userId);
    if (!userOpt) {
        result.code = 404;
        result.message = "用户不存在";
        return result;
    }

    auto &user = *userOpt;

    if (!req.username.empty() && req.username != user.username) {
        if (UserRepository::existsByUsername(req.username)) {
            result.code = 409;
            result.message = "用户名已存在";
            return result;
        }
        user.username = req.username;
    }

    if (!req.avatar.empty()) {
        user.avatar = req.avatar;
    }

    if (!UserRepository::update(user)) {
        result.code = 500;
        result.message = "更新用户失败";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "更新成功";
    result.data = user.toJson();
    return result;
}

ServiceResult UserService::changePassword(const std::string &userId, const ChangePasswordRequest &req) {
    ServiceResult result;

    auto userOpt = UserRepository::findById(userId);
    if (!userOpt) {
        result.code = 404;
        result.message = "用户不存在";
        return result;
    }

    auto &user = *userOpt;

    if (!PasswordUtils::verifyPassword(req.oldPassword, user.passwordHash)) {
        result.code = 400;
        result.message = "原密码错误";
        return result;
    }

    std::string newHash = PasswordUtils::hashPassword(req.newPassword);

    if (!UserRepository::updatePassword(userId, newHash)) {
        result.code = 500;
        result.message = "修改密码失败";
        return result;
    }


    result.success = true;
    result.code = 200;
    result.message = "密码修改成功";
    return result;
}