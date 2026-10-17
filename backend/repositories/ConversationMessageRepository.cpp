#include "ConversationMessageRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT id::text AS id, conversation_id::text AS conversation_id, role, message_type, "
    "       content, payload::text AS payload, created_at::text AS created_at "
    "FROM conversation_messages";

}  // namespace

bool ConversationMessageRepository::create(ConversationMessage &message) {
    try {
        auto db = drogon::app().getDbClient();
        const std::string payload =
            message.payload.is_object() ? message.payload.dump() : std::string("{}");
        auto result = db->execSqlSync(
            "INSERT INTO conversation_messages "
            "(conversation_id, role, message_type, content, payload) "
            "VALUES ($1, $2, $3, NULLIF($4, ''), $5::jsonb) "
            "RETURNING id::text, created_at::text",
            message.conversationId, message.role, message.messageType,
            message.content.value_or(""), payload);
        if (result.empty()) {
            return false;
        }
        message.id = std::string(result[0]["id"].as<std::string_view>());
        message.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
        return true;
    } catch (const std::exception &e) {
        spdlog::error("创建会话消息失败: {}", e.what());
        return false;
    }
}

std::optional<std::vector<ConversationMessage>> ConversationMessageRepository::findByConversation(
    const std::string &conversationId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) +
                " WHERE conversation_id = $1 ORDER BY created_at ASC, id ASC",
            conversationId);
        std::vector<ConversationMessage> messages;
        messages.reserve(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            messages.push_back(ConversationMessage::fromRow(result[i]));
        }
        return messages;
    } catch (const std::exception &e) {
        spdlog::error("查询会话消息失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<ConversationMessage> ConversationMessageRepository::findById(
    const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE id = $1", id);
        if (!result.empty()) {
            return ConversationMessage::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询会话消息失败: {}", e.what());
        return std::nullopt;
    }
}

bool ConversationMessageRepository::updatePayload(const std::string &id, const json &payload) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE conversation_messages SET payload = $2::jsonb WHERE id = $1",
            id, payload.is_object() ? payload.dump() : std::string("{}"));
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新会话消息失败: {}", e.what());
        return false;
    }
}
