#include "GenerationBatchRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT id::text AS id, conversation_id::text AS conversation_id, "
    "       message_id::text AS message_id, user_id::text AS user_id, status, style, size, "
    "       seed, panel_count, panels::text AS panels, consistency_prompt, negative_prompt, "
    "       parsed_data::text AS parsed_data, byok_llm, byok_image, "
    "       created_at::text AS created_at, updated_at::text AS updated_at "
    "FROM generation_batches";

std::string dumpJson(const json &value, const char *fallback) {
    return value.is_object() || value.is_array() ? value.dump() : std::string(fallback);
}

}  // namespace

bool GenerationBatchRepository::create(GenerationBatch &batch) {
    try {
        auto db = drogon::app().getDbClient();
        const std::string panels = dumpJson(batch.panels, "[]");
        const std::string parsed =
            batch.parsedData.has_value() ? batch.parsedData->dump() : std::string();

        auto result = db->execSqlSync(
            "INSERT INTO generation_batches "
            "(conversation_id, message_id, user_id, status, style, size, seed, panel_count, "
            " panels, consistency_prompt, negative_prompt, parsed_data, byok_llm, byok_image) "
            "VALUES (NULLIF($1, '')::uuid, NULLIF($2, '')::uuid, $3, $4, $5, NULLIF($6, ''), "
            "        NULLIF($7, -1), $8, $9::jsonb, NULLIF($10, ''), NULLIF($11, ''), "
            "        NULLIF($12, '')::jsonb, $13, $14) "
            "RETURNING id::text, created_at::text, updated_at::text",
            batch.conversationId.value_or(""),
            batch.messageId.value_or(""),
            batch.userId,
            batch.status,
            batch.style,
            batch.size.value_or(""),
            batch.seed.has_value() ? *batch.seed : -1,
            batch.panelCount,
            panels,
            batch.consistencyPrompt.value_or(""),
            batch.negativePrompt.value_or(""),
            parsed,
            batch.byokLlm,
            batch.byokImage);

        if (result.empty()) {
            return false;
        }
        batch.id = std::string(result[0]["id"].as<std::string_view>());
        batch.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
        batch.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
        return true;
    } catch (const std::exception &e) {
        spdlog::error("创建生成批次失败: {}", e.what());
        return false;
    }
}

std::optional<GenerationBatch> GenerationBatchRepository::findById(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE id = $1", id);
        if (!result.empty()) {
            return GenerationBatch::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询生成批次失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<GenerationBatch> GenerationBatchRepository::findByMessageId(
    const std::string &messageId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) + " WHERE message_id = $1 ORDER BY created_at DESC LIMIT 1",
            messageId);
        if (!result.empty()) {
            return GenerationBatch::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("按消息查询生成批次失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<std::vector<GenerationBatch>> GenerationBatchRepository::findByConversationId(
    const std::string &conversationId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) +
                " WHERE conversation_id = $1 ORDER BY created_at ASC",
            conversationId);
        std::vector<GenerationBatch> batches;
        batches.reserve(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            batches.push_back(GenerationBatch::fromRow(result[i]));
        }
        return batches;
    } catch (const std::exception &e) {
        spdlog::error("查询会话批次失败: {}", e.what());
        return std::nullopt;
    }
}

bool GenerationBatchRepository::updateDraft(GenerationBatch &batch) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_batches SET style = $2, size = NULLIF($3, ''), "
            "panel_count = $4, panels = $5::jsonb, "
            "consistency_prompt = NULLIF($6, ''), negative_prompt = NULLIF($7, ''), "
            "parsed_data = NULLIF($8, '')::jsonb, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = $1 AND status = 'draft'",
            batch.id,
            batch.style,
            batch.size.value_or(""),
            batch.panelCount,
            dumpJson(batch.panels, "[]"),
            batch.consistencyPrompt.value_or(""),
            batch.negativePrompt.value_or(""),
            batch.parsedData.has_value() ? batch.parsedData->dump() : std::string());
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新分镜草稿失败: {}", e.what());
        return false;
    }
}

bool GenerationBatchRepository::markStarted(const std::string &id, int seed) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_batches SET status = 'pending', seed = $2, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = $1 AND status = 'draft'",
            id, seed);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("启动生成批次失败: {}", e.what());
        return false;
    }
}

bool GenerationBatchRepository::updateStatus(const std::string &id, const std::string &status) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_batches SET status = $2, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = $1",
            id, status);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("更新批次状态失败: {}", e.what());
        return false;
    }
}

bool GenerationBatchRepository::refreshStatus(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_batches b SET status = CASE "
            "  WHEN NOT EXISTS (SELECT 1 FROM generation_tasks t "
            "                   WHERE t.batch_id = b.id AND t.status IN ('pending','processing')) "
            "  THEN CASE "
            "    WHEN EXISTS (SELECT 1 FROM generation_tasks t "
            "                 WHERE t.batch_id = b.id AND t.status = 'completed') "
            "    THEN CASE WHEN EXISTS (SELECT 1 FROM generation_tasks t "
            "                           WHERE t.batch_id = b.id "
            "                             AND t.status IN ('failed','cancelled')) "
            "              THEN 'partial' ELSE 'completed' END "
            "    WHEN EXISTS (SELECT 1 FROM generation_tasks t "
            "                 WHERE t.batch_id = b.id AND t.status = 'cancelled') "
            "    THEN 'cancelled' "
            "    ELSE 'failed' END "
            "  WHEN EXISTS (SELECT 1 FROM generation_tasks t "
            "               WHERE t.batch_id = b.id AND t.status = 'processing') "
            "  THEN 'processing' ELSE 'pending' END, "
            "updated_at = CURRENT_TIMESTAMP WHERE b.id = $1",
            id);
        return result.affectedRows() > 0;
    } catch (const std::exception &e) {
        spdlog::error("聚合批次状态失败: {}", e.what());
        return false;
    }
}

int GenerationBatchRepository::cancelPendingTasks(const std::string &id) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "UPDATE generation_tasks SET status = 'cancelled', "
            "completed_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP "
            "WHERE batch_id = $1 AND status = 'pending'",
            id);
        return static_cast<int>(result.affectedRows());
    } catch (const std::exception &e) {
        spdlog::error("取消批次任务失败: {}", e.what());
        return 0;
    }
}

int GenerationBatchRepository::countActiveByUser(const std::string &userId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "SELECT COUNT(*)::int AS cnt FROM generation_batches "
            "WHERE user_id = $1 AND status IN ('pending','processing')",
            userId);
        return result.empty() ? 0 : result[0]["cnt"].as<int>();
    } catch (const std::exception &e) {
        spdlog::error("统计进行中批次失败: {}", e.what());
        return 0;
    }
}
