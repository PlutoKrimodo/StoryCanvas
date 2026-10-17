#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../models/Conversation.h"

/** 会话数据访问：全部参数化 SQL；nullopt 表示数据库异常。 */
class ConversationRepository {
public:
    static bool create(Conversation &conversation);

    static std::optional<std::vector<Conversation>> findByUser(const std::string &userId,
                                                               int limit,
                                                               int offset);

    static std::optional<Conversation> findById(const std::string &id);

    /** 更新标题 / 风格 / 尺寸（空串表示保持原值）并刷新 updated_at。 */
    static bool updateMeta(const std::string &id,
                           const std::string &title,
                           const std::string &style,
                           const std::string &size);

    static bool touch(const std::string &id);

    static bool remove(const std::string &id, const std::string &userId);

    static int countByUser(const std::string &userId);
};
