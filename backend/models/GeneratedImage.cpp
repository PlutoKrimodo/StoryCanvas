#include "GeneratedImage.h"

#include "RowUtils.h"

namespace {
constexpr const char *kImageRawPrefix = "/api/v1/images/";
constexpr const char *kImageRawSuffix = "/raw";
}  // namespace

std::string GeneratedImage::imageUrl() const {
    return std::string(kImageRawPrefix) + id + kImageRawSuffix;
}

json GeneratedImage::toJson() const {
    return {
        {"id", id},
        {"task_id", taskId},
        {"file_path", filePath},
        {"image_url", imageUrl()},
        {"file_size", rowutil::toJsonValue(fileSize)},
        {"width", rowutil::toJsonValue(width)},
        {"height", rowutil::toJsonValue(height)},
        {"format", rowutil::toJsonValue(format)},
        {"prompt_used", rowutil::toJsonValue(promptUsed)},
        {"is_favorite", isFavorite},
        {"created_at", rowutil::toJsonValue(createdAt)}
    };
}

GeneratedImage GeneratedImage::fromRow(const drogon::orm::Row &row) {
    GeneratedImage image;
    image.id = rowutil::str(row["id"]);
    image.taskId = rowutil::str(row["task_id"]);
    image.userId = rowutil::str(row["user_id"]);
    image.originalUrl = rowutil::optString(row["original_url"]);
    image.filePath = rowutil::str(row["file_path"]);
    image.fileSize = rowutil::optLong(row["file_size"]);
    image.width = rowutil::optInt(row["width"]);
    image.height = rowutil::optInt(row["height"]);
    image.format = rowutil::optString(row["format"]);
    image.promptUsed = rowutil::optString(row["prompt_used"]);
    image.parameters = rowutil::jsonObjectOrEmpty(row["parameters"]);
    if (!row["is_favorite"].isNull()) {
        image.isFavorite = row["is_favorite"].as<bool>();
    }
    image.createdAt = rowutil::optString(row["created_at"]);
    return image;
}
