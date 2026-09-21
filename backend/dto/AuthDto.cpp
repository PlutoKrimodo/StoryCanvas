#include "AuthDto.h"
#include "../utils/PasswordUtils.h"

RegisterRequest RegisterRequest::fromJson(const json &j){
    RegisterRequest req;
    if (j.contains("username")) req.username = j["username"].get<std::string>();
    if (j.contains("email")) req.email = j["email"].get<std::string>();
    if (j.contains("password")) req.password = j["password"].get<std::string>();
    return req;
}

bool RegisterRequest::validate() const {
    return getErrors().empty();
}

//验证信息格式
std::vector<FieldError> RegisterRequest::getErrors() const {
    std::vector<FieldError> errors;
    
    if (username.empty()) {
        errors.push_back({"username", "用户名不能为空"});
    } else if (username.length() < 3 || username.length() > 50) {
        errors.push_back({"username", "用户名长度必须在 3-50 之间"});
    }
    
    if (email.empty()) {
        errors.push_back({"email", "邮箱不能为空"});
    } else if (email.find('@') == std::string::npos) {
        errors.push_back({"email", "邮箱格式无效"});
    }
    
    if (password.empty()) {
        errors.push_back({"password", "密码不能为空"});
    } else if (!PasswordUtils::isStrongPassword(password)) {
        errors.push_back({"password", "密码必须至少 8 位，且包含大小写字母和数字"});
    }
    
    return errors;
}

LoginRequest LoginRequest::fromJson(const json &j) {
    LoginRequest req;
    if (j.contains("email")) req.email = j["email"].get<std::string>();
    if (j.contains("password")) req.password = j["password"].get<std::string>();
    return req;
}

bool LoginRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> LoginRequest::getErrors() const {
    std::vector<FieldError> errors;
    
    if (email.empty()) {
        errors.push_back({"email", "邮箱不能为空"});
    }
    
    if (password.empty()) {
        errors.push_back({"password", "密码不能为空"});
    }
    
    return errors;
}

RefreshRequest RefreshRequest::fromJson(const json &j) {
    RefreshRequest req;
    if (j.contains("refresh_token")) req.refreshToken = j["refresh_token"].get<std::string>();
    return req;
}

bool RefreshRequest::validate() const {
    return !refreshToken.empty();
}

json UserInfo::toJson() const {
    return {
        {"id", id},
        {"username", username},
        {"email", email},
        {"avatar", avatar.empty() ? nlohmann::json(nullptr) : nlohmann::json(avatar)},
        {"created_at", createdAt}
    };
}

json AuthResponse::toJson() const {
    return {
        {"user", user.toJson()},
        {"access_token", accessToken},
        {"refresh_token", refreshToken}
    };
}