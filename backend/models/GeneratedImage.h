#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <drogon/orm/Field.h>

using json = nlohmann::json;

/** 生成图模型，对应 generated_images 表。 */
struct GeneratedImage {
    std::string id;
    std::string taskId;
    std::string userId;
    std::optional<std::string> originalUrl;
    std::string filePath;  // 相对存储根目录的路径
    std::optional<long long> fileSize;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<std::string> format;
    std::optional<std::string> promptUsed;
    json parameters = json::object();
    bool isFavorite = false;
    std::optional<std::string> createdAt;

    /** 对外访问地址（经后端转发，见 ImageController）。 */
    std::string imageUrl() const;

    json toJson() const;

    static GeneratedImage fromRow(const drogon::orm::Row &row);
};
