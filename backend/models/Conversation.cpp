#include "Conversation.h"

#include "RowUtils.h"

json Conversation::toJson() const {
    return {
        {"id", id},
        {"title", rowutil::toJsonValue(title)},
        {"style", rowutil::toJsonValue(style)},
        {"size", rowutil::toJsonValue(size)},
        {"created_at", createdAt},
        {"updated_at", updatedAt},
    };
}

Conversation Conversation::fromRow(const drogon::orm::Row &row) {
    Conversation conversation;
    conversation.id = rowutil::str(row["id"]);
    conversation.userId = rowutil::str(row["user_id"]);
    conversation.title = rowutil::optString(row["title"]);
    conversation.style = rowutil::optString(row["style"]);
    conversation.size = rowutil::optString(row["size"]);
    conversation.createdAt = rowutil::str(row["created_at"]);
    conversation.updatedAt = rowutil::str(row["updated_at"]);
    return conversation;
}
