#include "UserAiCredentialRepository.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace {

const char *kSelectColumns =
    "SELECT id::text AS id, user_id::text AS user_id, capability, provider, "
    "       api_key_enc, api_key_hint, api_base, model, "
    "       created_at::text AS created_at, updated_at::text AS updated_at "
    "FROM user_ai_credentials";

}  // namespace

std::optional<std::vector<UserAiCredential>> UserAiCredentialRepository::findByUser(
    const std::string &userId) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(std::string(kSelectColumns) + " WHERE user_id = $1",
                                      userId);
        std::vector<UserAiCredential> credentials;
        credentials.reserve(result.size());
        for (size_t i = 0; i < result.size(); ++i) {
            credentials.push_back(UserAiCredential::fromRow(result[i]));
        }
        return credentials;
    } catch (const std::exception &e) {
        spdlog::error("查询用户 AI 凭据失败: {}", e.what());
        return std::nullopt;
    }
}

std::optional<UserAiCredential> UserAiCredentialRepository::findByUserAndCapability(
    const std::string &userId, const std::string &capability) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            std::string(kSelectColumns) + " WHERE user_id = $1 AND capability = $2",
            userId, capability);
        if (!result.empty()) {
            return UserAiCredential::fromRow(result[0]);
        }
        return std::nullopt;
    } catch (const std::exception &e) {
        spdlog::error("查询用户 AI 凭据失败: {}", e.what());
        return std::nullopt;
    }
}

bool UserAiCredentialRepository::upsert(UserAiCredential &credential) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "INSERT INTO user_ai_credentials "
            "(user_id, capability, provider, api_key_enc, api_key_hint, api_base, model) "
            "VALUES ($1, $2, $3, $4, $5, NULLIF($6, ''), NULLIF($7, '')) "
            "ON CONFLICT (user_id, capability) DO UPDATE SET "
            "provider = EXCLUDED.provider, api_key_enc = EXCLUDED.api_key_enc, "
            "api_key_hint = EXCLUDED.api_key_hint, api_base = EXCLUDED.api_base, "
            "model = EXCLUDED.model, updated_at = CURRENT_TIMESTAMP "
            "RETURNING id::text, created_at::text, updated_at::text",
            credential.userId,
            credential.capability,
            credential.provider,
            credential.apiKeyEnc,
            credential.apiKeyHint,
            credential.apiBase.value_or(""),
            credential.model.value_or(""));

        if (result.empty()) {
            return false;
        }
        credential.id = std::string(result[0]["id"].as<std::string_view>());
        credential.createdAt = std::string(result[0]["created_at"].as<std::string_view>());
        credential.updatedAt = std::string(result[0]["updated_at"].as<std::string_view>());
        return true;
    } catch (const std::exception &e) {
        // 只记录数据库错误，不记录任何密文 / Key
        spdlog::error("保存用户 AI 凭据失败: {}", e.what());
        return false;
    }
}

UserAiCredentialRepository::RemoveResult UserAiCredentialRepository::remove(
    const std::string &userId,
    const std::string &capability) {
    try {
        auto db = drogon::app().getDbClient();
        auto result = db->execSqlSync(
            "DELETE FROM user_ai_credentials WHERE user_id = $1 AND capability = $2",
            userId, capability);
        return result.affectedRows() > 0 ? RemoveResult::Removed : RemoveResult::NotFound;
    } catch (const std::exception &e) {
        spdlog::error("删除用户 AI 凭据失败: {}", e.what());
        return RemoveResult::Error;
    }
}
