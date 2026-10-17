#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <drogon/orm/Field.h>

using json = nlohmann::json;

/**
 * 会话消息，对应 conversation_messages 表。
 *
 * payload 用于承载分镜、批次 ID、图片列表等可演进结构，
 * 不存放任何 API Key 或内部凭据。
 */
struct ConversationMessage {
    std::string id;
    std::string conversationId;
    std::string role;         // user | assistant
    std::string messageType;  // text | storyboard | images
    std::optional<std::string> content;
    json payload = json::object();
    std::string createdAt;

    json toJson() const;

    static ConversationMessage fromRow(const drogon::orm::Row &row);
};
