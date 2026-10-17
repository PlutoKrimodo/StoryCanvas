#include "ConversationRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT id::text AS id, user_id::text AS user_id, title, style, size, "
    "       created_at::text AS created_at, updated_at::text AS updated_at "
    "FROM conversations";

}  // namespace

bool ConversationRepository::create(Conversation &conversation) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "INSERT INTO conversations (user_id, title, style, size) "
            "VALUES ($1, NULLIF($2, ''), NULLIF($3, ''), NULLIF($4, '')) "
            "RETURNING id::text, created_at::text, updated_at::text",
            conversation.userId,
            conversation.title.value_or(""),
            conversation.style.value_or(""),
            conversation.size.value_or(""));
        if (result.empty()) {
            return false;
        }
        conversation.id = std::string(result[0]["id"].as<std::string_view>());
        conversation.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
        conversation.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
        return true;
    } catch (const std::exception &e) {
        spdlog::error("创建会话失败: {}", e.what());
        return false;
    }
}

std::optional<std::vector<Conversation>> ConversationRepository::findByUser(
    const std::string &userId, int limit, int offset) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) +
                " WHERE user_id = $1 ORDER BY updated_at DESC, id DESC "
                "LIMIT $2::int OFFSET $3::int",
            userId, limit, offset);
        std::vector<Conversation> items;
        items.reserve(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            items.push_back(Conversation::fromRow(result[i]));
        }
        return items;
    } catch (const std::exception &e) {
        spdlog::error("查询会话列表失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<Conversation> ConversationRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE id = $1", id);
        if (!result.empty()) {
            return Conversation::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询会话失败: {}", e.what());
        return std::nullopt;
    }
}

bool ConversationRepository::updateMeta(const std::string &id,
                                        const std::string &title,
                                        const std::string &style,
                                        const std::string &size) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE conversations SET "
            "title = COALESCE(NULLIF($2, ''), title), "
            "style = COALESCE(NULLIF($3, ''), style), "
            "size = COALESCE(NULLIF($4, ''), size), "
            "updated_at = CURRENT_TIMESTAMP WHERE id = $1",
            id, title, style, size);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新会话失败: {}", e.what());
        return false;
    }
}

bool ConversationRepository::touch(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE conversations SET updated_at = CURRENT_TIMESTAMP WHERE id = $1", id);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("刷新会话时间失败: {}", e.what());
        return false;
    }
}

bool ConversationRepository::remove(const std::string &id, const std::string &userId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "DELETE FROM conversations WHERE id = $1 AND user_id = $2", id, userId);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("删除会话失败: {}", e.what());
        return false;
    }
}

int ConversationRepository::countByUser(const std::string &userId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM conversations WHERE user_id = $1", userId);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计会话数量失败: {}", e.what());
        return 0;
    }
}
