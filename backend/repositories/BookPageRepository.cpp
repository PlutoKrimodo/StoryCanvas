#include "BookPageRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT p.id::text AS id, p.book_id::text AS book_id, p.page_number, p.title, p.thumbnail, "
    "       p.image_id::text AS image_id, g.file_path AS image_file_path, "
    "       p.created_at::text AS created_at, p.updated_at::text AS updated_at "
    "FROM book_pages p "
    "LEFT JOIN generated_images g ON g.id = p.image_id";

}  // namespace

bool BookPageRepository::upsert(BookPage &page) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "INSERT INTO book_pages (book_id, page_number, title, image_id) "
            "VALUES ($1, $2, NULLIF($3, ''), $4) "
            "ON CONFLICT (book_id, page_number) DO UPDATE "
            "SET title = EXCLUDED.title, image_id = EXCLUDED.image_id "
            "RETURNING id::text, created_at::text, updated_at::text",
            page.bookId,
            page.pageNumber,
            page.title.value_or(""),
            page.imageId.value_or(""));

        if (!result.empty()) {
            page.id = std::string(result[0]["id"].as<std::string_view>());
            page.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
            page.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("保存绘本页失败: {}", e.what());
        return false;
    }
}

std::vector<BookPage> BookPageRepository::findByBookId(const std::string &bookId) {
    std::vector<BookPage> pages;
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) + " WHERE p.book_id = $1 ORDER BY p.page_number ASC",
            bookId);
        for (size_t i = 0; i < result.size(); ++i) {
            pages.push_back(BookPage::fromRow(result[i]));
        }
    } catch (const std::exception &e) {
        spdlog::error("查询绘本页失败: {}", e.what());
    }
    return pages;
}

std::optional<BookPage> BookPageRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE p.id = $1", id);
        if (!result.empty()) {
            return BookPage::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询绘本页失败: {}", e.what());
        return std::nullopt;
    }
}

int BookPageRepository::countByBookId(const std::string &bookId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM book_pages WHERE book_id = $1", bookId);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计绘本页数失败: {}", e.what());
        return 0;
    }
}
