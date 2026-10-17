#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include <drogon/orm/Field.h>

using json = nlohmann::json;

/**
 * 用户自带 AI 凭据（user_ai_credentials 表）。
 *
 * 安全约定：
 *   - apiKeyEnc 只在服务层与数据库之间流转，`toPublicJson` 绝不输出；
 *   - 对外只暴露 apiKeyHint（sk-****abcd）。
 */
struct UserAiCredential {
    std::string id;
    std::string userId;
    std::string capability;  // llm | image
    std::string provider;    // deepseek / openai / doubao ...
    std::string apiKeyEnc;   // AES-256-GCM 密文
    std::string apiKeyHint;  // 脱敏展示
    std::optional<std::string> apiBase;
    std::optional<std::string> model;
    std::string createdAt;
    std::string updatedAt;

    /** 对外脱敏视图；不包含 apiKeyEnc。 */
    json toPublicJson() const;

    static UserAiCredential fromRow(const drogon::orm::Row &row);
};
