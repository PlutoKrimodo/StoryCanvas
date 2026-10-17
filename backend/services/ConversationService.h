#pragma once

#include <optional>
#include <string>

#include "../dto/ConversationDto.h"
#include "../models/Conversation.h"
#include "../models/ConversationMessage.h"
#include "ServiceResult.h"

/** 会话业务：归属校验、消息写入与结果回放。 */
class ConversationService {
public:
    static ServiceResult create(const std::string &userId,
                                const CreateConversationRequest &req);
    static ServiceResult list(const std::string &userId, int page, int limit);
    static ServiceResult get(const std::string &userId, const std::string &conversationId);
    static ServiceResult remove(const std::string &userId, const std::string &conversationId);

    /** 取已有会话或新建；失败时写入 result。 */
    static bool ensure(const std::string &userId,
                       const std::string &conversationId,
                       const std::string &title,
                       const std::string &style,
                       const std::string &size,
                       Conversation &out,
                       ServiceResult &result);

    /** 写入一条消息；失败返回 false（调用方决定是否回滚）。 */
    static bool append(const std::string &conversationId,
                       const std::string &role,
                       const std::string &messageType,
                       const std::string &content,
                       const json &payload,
                       ConversationMessage &out);

    static bool updateMessagePayload(const std::string &messageId, const json &payload);
};
