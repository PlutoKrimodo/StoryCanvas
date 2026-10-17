#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <drogon/orm/Field.h>

using json = nlohmann::json;

/**
 * 一次「分镜确认 → N 张图」的父记录，对应 generation_batches 表。
 *
 * 状态：draft（分镜草稿）/ pending / processing / completed / partial / failed / cancelled。
 * N 个 generation_tasks 共享本记录的 parsed_data、consistency_prompt、seed、风格。
 */
struct GenerationBatch {
    std::string id;
    std::optional<std::string> conversationId;
    std::optional<std::string> messageId;
    std::string userId;
    std::string status = "draft";
    std::string style = "cartoon";
    std::optional<std::string> size;
    std::optional<int> seed;
    int panelCount = 1;
    json panels = json::array();
    std::optional<std::string> consistencyPrompt;
    std::optional<std::string> negativePrompt;
    std::optional<json> parsedData;
    bool byokLlm = false;
    bool byokImage = false;
    std::string createdAt;
    std::string updatedAt;

    json toJson() const;

    static GenerationBatch fromRow(const drogon::orm::Row &row);
};
