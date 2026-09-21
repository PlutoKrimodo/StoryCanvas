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
    
    if (!row["avatar"].isNull()) {
        user.avatar = std::string(row["avatar"].as<std::string_view>());
    }
    
    user.isActive = row["is_active"].as<bool>();
    user.createdAt = std::string(row["created_at"].as<std::string_view>());
    user.updatedAt = std::string(row["updated_at"].as<std::string_view>());
    
    return user;
}