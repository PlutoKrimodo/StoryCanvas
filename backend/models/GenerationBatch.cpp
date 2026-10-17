#include "GenerationBatch.h"

#include "RowUtils.h"

json GenerationBatch::toJson() const {
    json parsed = nullptr;
    if (parsedData.has_value()) {
        parsed = *parsedData;
    }

    return {
        {"batch_id", id},
        {"conversation_id", rowutil::toJsonValue(conversationId)},
        {"message_id", rowutil::toJsonValue(messageId)},
        {"status", status},
        {"style", style},
        {"size", rowutil::toJsonValue(size)},
        {"seed", rowutil::toJsonValue(seed)},
        {"panel_count", panelCount},
        {"panels", panels.is_array() ? panels : json::array()},
        {"consistency_prompt", rowutil::toJsonValue(consistencyPrompt)},
        {"negative_prompt", rowutil::toJsonValue(negativePrompt)},
        {"parsed_data", parsed},
        {"byok_llm", byokLlm},
        {"byok_image", byokImage},
        {"created_at", createdAt},
        {"updated_at", updatedAt},
    };
}

GenerationBatch GenerationBatch::fromRow(const drogon::orm::Row &row) {
    GenerationBatch batch;
    batch.id = rowutil::str(row["id"]);
    batch.conversationId = rowutil::optString(row["conversation_id"]);
    batch.messageId = rowutil::optString(row["message_id"]);
    batch.userId = rowutil::str(row["user_id"]);
    batch.status = rowutil::str(row["status"]);
    batch.style = rowutil::str(row["style"]);
    batch.size = rowutil::optString(row["size"]);
    batch.seed = rowutil::optInt(row["seed"]);
    batch.panelCount = row["panel_count"].isNull() ? 1 : row["panel_count"].as<int>();
    batch.consistencyPrompt = rowutil::optString(row["consistency_prompt"]);
    batch.negativePrompt = rowutil::optString(row["negative_prompt"]);
    batch.parsedData = rowutil::optJson(row["parsed_data"]);
    batch.byokLlm = row["byok_llm"].as<bool>();
    batch.byokImage = row["byok_image"].as<bool>();

    auto panels = rowutil::optJson(row["panels"]);
    batch.panels = (panels.has_value() && panels->is_array()) ? *panels : json::array();

    batch.createdAt = rowutil::str(row["created_at"]);
    batch.updatedAt = rowutil::str(row["updated_at"]);
    return batch;
}
