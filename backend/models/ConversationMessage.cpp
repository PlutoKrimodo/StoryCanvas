#include "ConversationMessage.h"

#include "RowUtils.h"

json ConversationMessage::toJson() const {
    return {
        {"id", id},
        {"conversation_id", conversationId},
        {"role", role},
        {"message_type", messageType},
        {"content", rowutil::toJsonValue(content)},
        {"payload", payload.is_object() ? payload : json::object()},
        {"created_at", createdAt},
    };
}

ConversationMessage ConversationMessage::fromRow(const drogon::orm::Row &row) {
    ConversationMessage message;
    message.id = rowutil::str(row["id"]);
    message.conversationId = rowutil::str(row["conversation_id"]);
    message.role = rowutil::str(row["role"]);
    message.messageType = rowutil::str(row["message_type"]);
    message.content = rowutil::optString(row["content"]);
    message.payload = rowutil::jsonObjectOrEmpty(row["payload"]);
    message.createdAt = rowutil::str(row["created_at"]);
    return message;
}
