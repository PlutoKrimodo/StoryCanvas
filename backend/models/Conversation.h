#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <drogon/orm/Field.h>

using json = nlohmann::json;

/** 对话式创作会话，对应 conversations 表。游客不落库。 */
struct Conversation {
    std::string id;
    std::string userId;
    std::optional<std::string> title;
    std::optional<std::string> style;
    std::optional<std::string> size;
    std::string createdAt;
    std::string updatedAt;

    json toJson() const;

    static Conversation fromRow(const drogon::orm::Row &row);
};
