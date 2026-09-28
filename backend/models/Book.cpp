#include "Book.h"

json Book::toJson() const {
    // 注意：可能为空的字段必须显式构造 json 类型，
    // 避免 cond ? nullptr : std::string 被推导为 std::string 后崩溃（见开发记录问题 7）
    return {
        {"id", id},
        {"user_id", userId},
        {"title", title},
        {"description", description.has_value() ? nlohmann::json(*description) : nlohmann::json(nullptr)},
        {"cover_image", coverImage.has_value() ? nlohmann::json(*coverImage) : nlohmann::json(nullptr)},
        {"status", status},
        {"created_at", createdAt},
        {"updated_at", updatedAt}
    };
}

Book Book::fromRow(const drogon::orm::Row &row) {
    Book book;
    book.id = row["id"].as<std::string>();
    book.userId = row["user_id"].as<std::string>();
    book.title = row["title"].as<std::string>();

    // 空串与 NULL 统一视为"无值"，保持接口返回口径一致（见开发记录问题 13）
    if (!row["description"].isNull()) {
        const std::string value(row["description"].as<std::string_view>());
        if (!value.empty()) {
            book.description = value;
        }
    }
    if (!row["cover_image"].isNull()) {
        const std::string value(row["cover_image"].as<std::string_view>());
        if (!value.empty()) {
            book.coverImage = value;
        }
    }

    book.status = row["status"].as<std::string>();
    book.createdAt = std::string(row["created_at"].as<std::string_view>());
    book.updatedAt = std::string(row["updated_at"].as<std::string_view>());
    return book;
}
