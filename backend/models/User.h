#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <drogon/orm/Field.h>

using json = nlohmann::json;

//用户数据模型，与数据库的User表一一对应
struct User{
    std::string id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::optional<std::string> avatar;
    bool isActive = true;
    std::string createdAt;
    std::string updatedAt;

    //序列化为 JSON，不包含passwordHash
    json toJson() const;

    // 从数据库行反序列化
    static User fromRow(const drogon::orm::Row &row);
};

