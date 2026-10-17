#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../models/ConversationMessage.h"

/** 会话消息数据访问。 */
class ConversationMessageRepository {
public:
    static bool create(ConversationMessage &message);

    static std::optional<std::vector<ConversationMessage>> findByConversation(
        const std::string &conversationId);

    static std::optional<ConversationMessage> findById(const std::string &id);

    static bool updatePayload(const std::string &id, const json &payload);
};
