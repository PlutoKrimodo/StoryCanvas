#include "GenerationTask.h"

#include "RowUtils.h"

json GenerationTask::toJson() const {
    // result 仅在存在生成图时返回（与 docs/04 §8.2 契约一致）
    json result = nullptr;
    if (imageId.has_value()) {
        result = {
            {"image_id", *imageId},
            {"image_url", rowutil::toJsonValue(resultUrl)},
            {"width", rowutil::toJsonValue(imageWidth)},
            {"height", rowutil::toJsonValue(imageHeight)}
        };
    }

    return {
        {"task_id", id},
        {"user_id", userId},
        {"book_id", rowutil::toJsonValue(bookId)},
        {"page_id", rowutil::toJsonValue(pageId)},
        {"status", status},
        {"prompt", prompt},
        {"original_text", rowutil::toJsonValue(originalText)},
        {"parsed_data", parsedData.has_value() ? *parsedData : json(nullptr)},
        {"result", result},
        {"error_message", rowutil::toJsonValue(errorMessage)},
        {"model_used", rowutil::toJsonValue(modelUsed)},
        {"parameters", parameters},
        {"created_at", rowutil::toJsonValue(createdAt)},
        {"updated_at", rowutil::toJsonValue(updatedAt)},
        {"completed_at", rowutil::toJsonValue(completedAt)}
    };
}

GenerationTask GenerationTask::fromRow(const drogon::orm::Row &row) {
    GenerationTask task;
    task.id = rowutil::str(row["id"]);
    task.userId = rowutil::str(row["user_id"]);
    task.bookId = rowutil::optString(row["book_id"]);
    task.pageId = rowutil::optString(row["page_id"]);
    task.prompt = rowutil::str(row["prompt"]);
    task.originalText = rowutil::optString(row["original_text"]);
    task.parsedData = rowutil::optJson(row["parsed_data"]);
    task.status = rowutil::str(row["status"]);
    task.resultUrl = rowutil::optString(row["result_url"]);
    task.errorMessage = rowutil::optString(row["error_message"]);
    task.modelUsed = rowutil::optString(row["model_used"]);
    task.parameters = rowutil::jsonObjectOrEmpty(row["parameters"]);
    task.createdAt = rowutil::optString(row["created_at"]);
    task.updatedAt = rowutil::optString(row["updated_at"]);
    task.completedAt = rowutil::optString(row["completed_at"]);

    task.imageId = rowutil::optString(row["image_id"]);
    task.imageWidth = rowutil::optInt(row["image_width"]);
    task.imageHeight = rowutil::optInt(row["image_height"]);
    return task;
}
