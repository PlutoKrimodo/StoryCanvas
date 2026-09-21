#include "UserDto.h"
#include "../utils/PasswordUtils.h"

UpdateUserRequest UpdateUserRequest::fromJson(const json &j) {
    UpdateUserRequest req;
    if (j.contains("username")) req.username = j["username"].get<std::string>();
    if (j.contains("avatar")) req.avatar = j["avatar"].get<std::string>();
    return req;
}

bool UpdateUserRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> UpdateUserRequest::getErrors() const {
    std::vector<FieldError> errors;
    
    if (!username.empty() && (username.length() < 3 || username.length() > 50)) {
        errors.push_back({"username", "用户名长度必须在 3-50 之间"});
    }
    
    if (!avatar.empty() && avatar.length() > 500) {
        errors.push_back({"avatar", "头像 URL 不能超过 500 字符"});
    }
    
    return errors;
}

ChangePasswordRequest ChangePasswordRequest::fromJson(const json &j) {
    ChangePasswordRequest req;
    if (j.contains("old_password")) req.oldPassword = j["old_password"].get<std::string>();
    if (j.contains("new_password")) req.newPassword = j["new_password"].get<std::string>();
    return req;
}

bool ChangePasswordRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> ChangePasswordRequest::getErrors() const {
    std::vector<FieldError> errors;
    
    if (oldPassword.empty()) {
        errors.push_back({"old_password", "原密码不能为空"});
    }
    
    if (newPassword.empty()) {
        errors.push_back({"new_password", "新密码不能为空"});
    } else if (!PasswordUtils::isStrongPassword(newPassword)) {
        errors.push_back({"new_password", "新密码必须至少 8 位，且包含大小写字母和数字"});
    }
    
    if (!oldPassword.empty() && !newPassword.empty() && oldPassword == newPassword) {
        errors.push_back({"new_password", "新密码不能与原密码相同"});
    }
    
    return errors;
}

json UserResponse::toJson() const {
    return {
        {"id", id},
        {"username", username},
        {"email", email},
        {"avatar", avatar.empty() ? nlohmann::json(nullptr) : nlohmann::json(avatar)},
        {"created_at", createdAt}
    };
}