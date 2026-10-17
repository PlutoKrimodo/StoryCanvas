#include "UserAiCredential.h"

#include "RowUtils.h"
#include "../utils/AiCredentialRules.h"

json UserAiCredential::toPublicJson() const {
    AiCapability cap = AiCapability::Unknown;
    AiCredentialRules::parseCapability(capability, cap);

    return {
        {"capability", capability},
        {"capability_label", AiCredentialRules::capabilityLabel(cap)},
        {"provider", provider},
        {"provider_label", AiCredentialRules::providerLabel(cap, provider)},
        {"api_key_hint", apiKeyHint},
        {"api_base", rowutil::toJsonValue(apiBase)},
        {"model", rowutil::toJsonValue(model)},
        {"updated_at", updatedAt},
    };
}

UserAiCredential UserAiCredential::fromRow(const drogon::orm::Row &row) {
    UserAiCredential credential;
    credential.id = rowutil::str(row["id"]);
    credential.userId = rowutil::str(row["user_id"]);
    credential.capability = rowutil::str(row["capability"]);
    credential.provider = rowutil::str(row["provider"]);
    credential.apiKeyEnc = rowutil::str(row["api_key_enc"]);
    credential.apiKeyHint = rowutil::str(row["api_key_hint"]);
    credential.apiBase = rowutil::optString(row["api_base"]);
    credential.model = rowutil::optString(row["model"]);
    credential.createdAt = rowutil::str(row["created_at"]);
    credential.updatedAt = rowutil::str(row["updated_at"]);
    return credential;
}
