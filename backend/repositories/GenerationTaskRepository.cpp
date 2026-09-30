#include "GenerationTaskRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

/**
 * 统一查询投影。
 * - 时间戳转 text 交给前端归一化（见开发记录问题 12）
 * - JSONB 转 text 由 C++ 侧解析
 * - LEFT JOIN LATERAL 取"最近一张生成图"，保证一对一不放大行数
 */
const char *kSelectColumns =
    "SELECT t.id::text AS id, t.user_id::text AS user_id, t.book_id::text AS book_id, "
    "       t.page_id::text AS page_id, t.prompt, t.original_text, "
    "       t.parsed_data::text AS parsed_data, t.status, t.result_url, t.error_message, "
    "       t.model_used, t.parameters::text AS parameters, "
    "       t.created_at::text AS created_at, t.updated_at::text AS updated_at, "
    "       t.completed_at::text AS completed_at, "
    "       g.id::text AS image_id, g.width AS image_width, g.height AS image_height "
    "FROM generation_tasks t "
    "LEFT JOIN LATERAL ("
    "    SELECT gi.id, gi.width, gi.height FROM generated_images gi "
    "    WHERE gi.task_id = t.id ORDER BY gi.created_at DESC LIMIT 1"
    ") g ON true";

}  // namespace

bool GenerationTaskRepository::create(GenerationTask &task) {
    try {
        auto db = drogon::app().getDbClient();
        const std::string parametersDump =
            task.parameters.is_object() ? task.parameters.dump() : std::string("{}");

        auto result = db->execSqlSync(
            "INSERT INTO generation_tasks "
            "(user_id, book_id, page_id, original_text, prompt, status, parameters) "
            "VALUES ($1, NULLIF($2, '')::uuid, NULLIF($3, '')::uuid, NULLIF($4, ''), $5, 'pending', $6::jsonb) "
            "RETURNING id::text, created_at::text, updated_at::text",
            task.userId,
            task.bookId.value_or(""),
            task.pageId.value_or(""),
            task.originalText.value_or(""),
            task.prompt,
            parametersDump);

        if (!result.empty()) {
            task.id = std::string(result[0]["id"].as<std::string_view>());
            task.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
            task.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
            task.status = "pending";
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("创建生成任务失败: {}", e.what());
        return false;
    }
}

bool GenerationTaskRepository::updateResult(GenerationTask &task) {
    try {
        auto db = drogon::app().getDbClient();
        const std::string parsedDump =
            task.parsedData.has_value() ? task.parsedData->dump() : std::string();

        auto result = db->execSqlSync(
            "UPDATE generation_tasks SET "
            "status = $1, prompt = $2, parsed_data = NULLIF($3, '')::jsonb, "
            "result_url = NULLIF($4, ''), error_message = NULLIF($5, ''), "
            "model_used = NULLIF($6, ''), "
            "completed_at = CASE WHEN $1 IN ('completed', 'failed', 'cancelled') "
            "                    THEN CURRENT_TIMESTAMP ELSE completed_at END "
            "WHERE id = $7 "
            "RETURNING updated_at::text, completed_at::text",
            task.status,
            task.prompt,
            parsedDump,
            task.resultUrl.value_or(""),
            task.errorMessage.value_or(""),
            task.modelUsed.value_or(""),
            task.id);

        if (!result.empty()) {
            task.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
            if (!result[0]["completed_at"].isNull()) {
                task.completedAt =
                    std::string(result[0]["completed_at"].as<std::string_view>());
            }
            return true;
        }
        return false;
    } catch (const std::exception &e) {
        spdlog::error("更新生成任务失败: {}", e.what());
        return false;
    }
}

bool GenerationTaskRepository::markStatus(const std::string &id, const std::string &status) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_tasks SET status = $1 WHERE id = $2", status, id);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新任务状态失败: {}", e.what());
        return false;
    }
}

std::optional<GenerationTask> GenerationTaskRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE t.id = $1", id);
        if (!result.empty()) {
            return GenerationTask::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询生成任务失败: {}", e.what());
        return std::nullopt;
    }
}

std::vector<GenerationTask> GenerationTaskRepository::findByUserId(const std::string &userId,
                                                                   const std::string &bookId,
                                                                   int limit,
                                                                   int offset) {
    std::vector<GenerationTask> tasks;
    try {
        auto db = drogon::app().getDbClient();
        const std::string base(kSelectColumns);

        if (bookId.empty()) {
            auto result = db->execSqlSync(
                base + " WHERE t.user_id = $1 ORDER BY t.created_at DESC, t.id DESC "
                       "LIMIT $2 OFFSET $3",
                userId, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                tasks.push_back(GenerationTask::fromRow(result[i]));
            }
        } else {
            auto result = db->execSqlSync(
                base + " WHERE t.user_id = $1 AND t.book_id = $2::uuid "
                       "ORDER BY t.created_at DESC, t.id DESC LIMIT $3 OFFSET $4",
                userId, bookId, limit, offset);
            for (size_t i = 0; i < result.size(); ++i) {
                tasks.push_back(GenerationTask::fromRow(result[i]));
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("查询生成历史失败: {}", e.what());
    }
    return tasks;
}

int GenerationTaskRepository::countByUserId(const std::string &userId,
                                            const std::string &bookId) {
    try {
        auto db = drogon::app().getDbClient();
        if (bookId.empty()) {
            auto result = db->execSqlSync(
                "SELECT COUNT(*)::int AS cnt FROM generation_tasks WHERE user_id = $1", userId);
            return result.empty() ? 0 : result[0]["cnt"].as<int>();
        }
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM generation_tasks "
            "WHERE user_id = $1 AND book_id = $2::uuid",
            userId, bookId);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计生成任务数量失败: {}", e.what());
        return 0;
    }
}

int GenerationTaskRepository::countByStatus(const std::string &status) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM generation_tasks WHERE status = $1", status);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计任务状态失败: {}", e.what());
        return 0;
    }
}

int GenerationTaskRepository::failStaleTasks() {
    try {
        auto db = drogon::app().getDbClient();
        // 进程重启后，内存队列已丢失，把遗留的 pending/processing 任务标记为失败，
        // 避免前端永远轮询不到结果
        auto result = db->execSqlSync(
            "UPDATE generation_tasks SET status = 'failed', "
            "error_message = '服务重启导致任务中断，请重新生成' "
            "WHERE status IN ('pending', 'processing')");
        return static_cast<int>(result.affectedRows());
    } catch (const std::exception &e) {
        spdlog::error("清理遗留任务失败: {}", e.what());
        return 0;
    }
}
