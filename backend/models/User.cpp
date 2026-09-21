#include "User.h"

json User::toJson() const{
    return{
        {"id", id},
        {"username", username},
        {"email", email},
        {"avatar", avatar.has_value() ? nlohmann::json(*avatar) : nlohmann::json(nullptr)},
        {"is_active", isActive},
        {"created_at", createdAt},
        {"updated_at", updatedAt}
    };
}

User User::fromRow(const drogon::orm::Row &row) {
    User user;
    user.id = row["id"].as<std::string>();
    user.username = row["username"].as<std::string>();
    user.email = row["email"].as<std::string>();
    user.passwordHash = row["password_hash"].as<std::string>();
    
    // 空串与 NULL 统一视为"无头像"，保证与其他接口返回 null 的口径一致
    if (!row["avatar"].isNull()) {
        const std::string avatarValue(row["avatar"].as<std::string_view>());
        if (!avatarValue.empty()) {
            user.avatar = avatarValue;
        }
    }
    
    user.isActive = row["is_active"].as<bool>();
    user.createdAt = std::string(row["created_at"].as<std::string_view>());
    user.updatedAt = std::string(row["updated_at"].as<std::string_view>());
    
    return user;
}