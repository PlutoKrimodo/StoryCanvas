#include "BookRepository.h"
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

// 统一查询列（id/user_id 显式转 text，时间戳转 text 交给前端归一化）
const char *kSelectColumns =
    "SELECT id::text, user_id::text, title, description, cover_image, status, "
    "created_at::text, updated_at::text FROM books";

}  // namespace

bool BookRepository::create(Book &book) {
    try {
        auto db = drogon::app().getDbClient();
        spdlog::debug("执行绘本插入: title={}", book.title);
        auto result = db->execSqlSync(
            "INSERT INTO books (user_id, title, description, status) "
            "VALUES ($1, $2, NULLIF($3, ''), $4) "
            "RETURNING id::text, created_at::text, updated_at::text",
            book.userId,
            book.title,
            book.description.value_or(""),
            book.status);

        if (!result.empty()) {
            book.id = std::string(result[0]["id"].as<std::string_view>());
            book.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
            book.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
            spdlog::debug("绘本创建完成: id={}", book.id);
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("绘本创建失败: {}", e.what());
        return false;
    }
}

std::optional<Book> BookRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) + " WHERE id = $1", id);
        if (!result.empty()) {
            return Book::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查找绘本失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<std::vector<Book>> BookRepository::findByUserId(const std::string &userId,
                                                             const std::string &status,
                                                             int limit,
                                                             int offset) {
    std::vector<Book> books;
    try {
        auto db = drogon::app().getDbClient();
        const std::string base(kSelectColumns);

        // LIMIT/OFFSET 的占位符必须显式转 int4：PostgreSQL 对 LIMIT/OFFSET 默认把参数推断为
        // bigint(8 字节)，而 Drogon 的整型绑定按 sizeof(int)=4 字节二进制发送（format=1），
        // 两者不匹配会报 "insufficient data left in message"（见 docs/开发记录.md 问题 24）。
        // 注意：写成 ::bigint 无效——客户端依旧只发 4 字节。
        if (status.empty()) {
            auto result = db->execSqlSync(
                base + " WHERE user_id = $1 "
                       "ORDER BY created_at DESC, id DESC LIMIT $2::int OFFSET $3::int",
                userId, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                books.push_back(Book::fromRow(result[i]));
            }
        } else {
            auto result = db->execSqlSync(
                base + " WHERE user_id = $1 AND status = $2 "
                       "ORDER BY created_at DESC, id DESC LIMIT $3::int OFFSET $4::int",
                userId, status, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                books.push_back(Book::fromRow(result[i]));
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("查询绘本列表失败: {}", e.what());
        // 查询失败不能伪装成空列表，交由服务层返回 500
        return std::nullopt;
    }
    return books;
}

int BookRepository::countByUserId(const std::string &userId, const std::string &status) {
    try {
        auto db = drogon::app().getDbClient();
        if (status.empty()) {
            auto result = db->execSqlSync(
                "SELECT COUNT(*)::int AS cnt FROM books WHERE user_id = $1", userId);
            return result.empty() ? 0 : result[0]["cnt"].as<int>();
        }
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM books WHERE user_id = $1 AND status = $2",
            userId, status);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计绘本数量失败: {}", e.what());
        return 0;
    }
}

bool BookRepository::update(Book &book) {
    try {
        auto db = drogon::app().getDbClient();
        // WHERE 同时限定 user_id，作为归属校验的数据库层兜底
        auto result = db->execSqlSync(
            "UPDATE books SET title = $1, description = NULLIF($2, ''), "
            "status = $3, cover_image = NULLIF($4, '') "
            "WHERE id = $5 AND user_id = $6 "
            "RETURNING updated_at::text",
            book.title,
            book.description.value_or(""),
            book.status,
            book.coverImage.value_or(""),
            book.id,
            book.userId);

        if (!result.empty()) {
            book.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("更新绘本失败: {}", e.what());
        return false;
    }
}

bool BookRepository::deleteById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync("DELETE FROM books WHERE id = $1", id);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("删除绘本失败: {}", e.what());
        return false;
    }
}
