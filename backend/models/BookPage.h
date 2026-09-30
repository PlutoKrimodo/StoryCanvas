#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <drogon/orm/Field.h>

using json = nlohmann::json;

/**
 * 页容器模型，对应 book_pages 表。
 *
 * imageFilePath 为关联生成图的路径，由 Repository 通过 LEFT JOIN 填充；
 * imageUrl 在 fromRow 阶段即派生出对外访问地址，避免上层重复拼接。
 */
struct BookPage {
    std::string id;
    std::string bookId;
    int pageNumber = 1;
    std::optional<std::string> title;
    std::optional<std::string> thumbnail;
    std::optional<std::string> imageId;
    std::optional<std::string> imageFilePath;
    std::optional<std::string> createdAt;
    std::optional<std::string> updatedAt;

    json toJson() const;

    static BookPage fromRow(const drogon::orm::Row &row);
};
