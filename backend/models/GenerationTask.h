#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <drogon/orm/Field.h>

using json = nlohmann::json;

/**
 * 生成任务模型，对应 generation_tasks 表。
 *
 * imageId / imageWidth / imageHeight 为「关联生成图」的派生字段，
 * 由 Repository 通过 LEFT JOIN LATERAL 填充，不是表字段。
 */
struct GenerationTask {
    std::string id;
    std::string userId;
    std::optional<std::string> bookId;
    std::optional<std::string> pageId;
    std::string prompt;
    std::optional<std::string> originalText;
    std::optional<json> parsedData;
    std::string status = "pending";
    std::optional<std::string> resultUrl;
    std::optional<std::string> errorMessage;
    std::optional<std::string> modelUsed;
    json parameters = json::object();
    std::optional<std::string> createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> completedAt;

    // 派生字段（最近一张生成图）
    std::optional<std::string> imageId;
    std::optional<int> imageWidth;
    std::optional<int> imageHeight;

    json toJson() const;

    static GenerationTask fromRow(const drogon::orm::Row &row);
};
