#include "GeneratedImageRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT i.id::text AS id, i.task_id::text AS task_id, i.user_id::text AS user_id, "
    "       i.original_url, i.file_path, i.file_size, i.width, i.height, i.format, "
    "       i.prompt_used, i.parameters::text AS parameters, i.is_favorite, "
    "       i.created_at::text AS created_at "
    "FROM generated_images i";

}  // namespace

bool GeneratedImageRepository::create(GeneratedImage &image) {
    try {
        auto db = drogon::app().getDbClient();
        const std::string parametersDump =
            image.parameters.is_object() ? image.parameters.dump() : std::string("{}");

        auto result = db->execSqlSync(
            "INSERT INTO generated_images "
            "(task_id, user_id, original_url, file_path, file_size, width, height, format, "
            " prompt_used, parameters) "
            "VALUES ($1, $2, NULLIF($3, ''), $4, NULLIF($5, 0)::int, NULLIF($6, 0)::int, "
            "        NULLIF($7, 0)::int, NULLIF($8, ''), NULLIF($9, ''), $10::jsonb) "
            "RETURNING id::text, created_at::text",
            image.taskId,
            image.userId,
            image.originalUrl.value_or(""),
            image.filePath,
            static_cast<int>(image.fileSize.value_or(0)),
            image.width.value_or(0),
            image.height.value_or(0),
            image.format.value_or(""),
            image.promptUsed.value_or(""),
            parametersDump);

        if (!result.empty()) {
            image.id = std::string(result[0]["id"].as<std::string_view>());
            image.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("保存生成图记录失败: {}", e.what());
        return false;
    }
}

std::optional<GeneratedImage> GeneratedImageRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE i.id = $1", id);
        if (!result.empty()) {
            return GeneratedImage::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询生成图失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<GeneratedImage> GeneratedImageRepository::findByIdAndUser(
    const std::string &id, const std::string &userId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) + " WHERE i.id = $1 AND i.user_id = $2", id, userId);
        if (!result.empty()) {
            return GeneratedImage::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询用户生成图失败: {}", e.what());
        return std::nullopt;
    }
}

std::vector<GeneratedImage> GeneratedImageRepository::findByUserId(const std::string &userId,
                                                                   const std::string &bookId,
                                                                   int limit,
                                                                   int offset) {
    std::vector<GeneratedImage> images;
    try {
        auto db = drogon::app().getDbClient();
        const std::string base(kSelectColumns);

        if (bookId.empty()) {
            auto result = db->execSqlSync(
                base + " WHERE i.user_id = $1 "
                       "ORDER BY i.created_at DESC, i.id DESC LIMIT $2 OFFSET $3",
                userId, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                images.push_back(GeneratedImage::fromRow(result[i]));
            }
        } else {
            auto result = db->execSqlSync(
                base +
                    " JOIN generation_tasks t ON t.id = i.task_id "
                    " WHERE i.user_id = $1 AND t.book_id = $2::uuid "
                    "ORDER BY i.created_at DESC, i.id DESC LIMIT $3 OFFSET $4",
                userId, bookId, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                images.push_back(GeneratedImage::fromRow(result[i]));
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("查询生成图列表失败: {}", e.what());
    }
    return images;
}

int GeneratedImageRepository::countByUserId(const std::string &userId,
                                            const std::string &bookId) {
    try {
        auto db = drogon::app().getDbClient();
        if (bookId.empty()) {
            auto result = db->execSqlSync(
                "SELECT COUNT(*)::int AS cnt FROM generated_images WHERE user_id = $1", userId);
            return result.empty() ? 0 : result[0]["cnt"].as<int>();
        }
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM generated_images i "
            "JOIN generation_tasks t ON t.id = i.task_id "
            "WHERE i.user_id = $1 AND t.book_id = $2::uuid",
            userId, bookId);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计生成图数量失败: {}", e.what());
        return 0;
    }
}

bool GeneratedImageRepository::deleteById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync("DELETE FROM generated_images WHERE id = $1", id);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("删除生成图失败: {}", e.what());
        return false;
    }
}
