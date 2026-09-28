#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <drogon/orm/Field.h>

using json = nlohmann::json;

// 绘本数据模型，与数据库 books 表一一对应
struct Book {
    std::string id;
    std::string userId;
    std::string title;
    std::optional<std::string> description;
    std::optional<std::string> coverImage;
    std::string status = "draft";
    std::string createdAt;
    std::string updatedAt;

    // 序列化为 JSON（供接口响应使用）
    json toJson() const;

    // 从数据库行反序列化
    static Book fromRow(const drogon::orm::Row &row);
};
