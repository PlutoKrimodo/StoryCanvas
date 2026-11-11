#include "UserAiCredentialService.h"

#include <spdlog/spdlog.h>

#include "../repositories/UserAiCredentialRepository.h"
#include "../utils/AiCredentialRules.h"
#include "../utils/AiCredentialTicket.h"
#include "../utils/ConfigManager.h"
#include "../utils/EncryptionUtils.h"
#include "../utils/SecretBox.h"
#include "AiServiceClient.h"

namespace {

std::string stringField(const json &body, const char *key) {
    if (!body.is_object() || !body.contains(key)) {
        return "";
    }
    const auto &value = body.at(key);
    return value.is_string() ? value.get<std::string>() : "";
}

ServiceResult fail(int code, const std::string &message) {
    ServiceResult result;
    result.code = code;
    result.message = message;
    return result;
}

ServiceResult ok(const std::string &message, const json &data) {
    ServiceResult result;
    result.success = true;
    result.code = 200;
    result.message = message;
    result.data = data;
    return result;
}

constexpr char kEncryptionMissing[] =
    "服务端未配置 ENCRYPTION_KEY，暂不能保存或测试用户的 API Key；"
    "请联系管理员配置后重试";

bool encryptionReady() {
    return SecretBox::hasKey(ConfigManager::encryptionKey);
}

std::string secretHint(const std::string &apiKey) {
    return SecretBox::hint(apiKey);
}

}  // namespace

json AiCredentialSecret::toRequestJson() const {
    json payload = {
        {"provider", provider},
        {"api_key", apiKey}
    };
    if (!apiBase.empty()) payload["api_base"] = apiBase;
    if (!model.empty()) payload["model"] = model;
    return payload;
}

ServiceResult UserAiCredentialService::overview(const std::string &userId) {
    if (!ConfigManager::byokEnabled) {
        return fail(404, "接口不存在");
    }

    auto credentials = UserAiCredentialRepository::findByUser(userId);
    if (!credentials.has_value()) {
        return fail(500, "查询 API Key 配置失败，请稍后重试");
    }

    json llm = nullptr;
    json image = nullptr;
    bool llmConfigured = false;
    bool imageConfigured = false;
    for (const auto &credential : *credentials) {
        if (credential.capability == "llm") {
            llm = credential.toPublicJson();
            llmConfigured = true;
        } else if (credential.capability == "image") {
            image = credential.toPublicJson();
            imageConfigured = true;
        }
    }

    return ok("success", {
        {"byok_enabled", true},
        {"encryption_ready", encryptionReady()},
        {"quota_exempt", AiCredentialRules::shouldExemptQuota(llmConfigured, imageConfigured)},
        {"credentials", {
            {"llm", llm},
            {"image", image}
        }}
    });
}

ServiceResult UserAiCredentialService::providers() {
    if (!ConfigManager::byokEnabled) {
        return fail(404, "接口不存在");
    }
    return ok("success", AiCredentialRules::catalogJson());
}

bool UserAiCredentialService::prepare(const std::string &userId,
                                      const std::string &capabilityText,
                                      const json &body,
                                      PreparedCredential &out,
                                      ServiceResult &result) {
    AiCapability capability = AiCapability::Unknown;
    if (!AiCredentialRules::parseCapability(capabilityText, capability)) {
        result = fail(422, "capability 仅支持 llm 或 image");
        return false;
    }
    if (!body.is_object()) {
        result = fail(400, "请求体必须是 JSON 对象");
        return false;
    }

    const std::string provider = stringField(body, "provider");
    if (provider.empty()) {
        result = fail(422, "请选择厂商");
        return false;
    }
    const auto *preset = AiCredentialRules::findProvider(capability, provider);
    if (preset == nullptr) {
        result = fail(422, "不支持的厂商：" + provider);
        return false;
    }

    std::string apiKey = stringField(body, "api_key");
    if (apiKey.empty()) {
        // 更新时允许不重填 Key：沿用旧密文解密后的值
        auto existing =
            UserAiCredentialRepository::findByUserAndCapability(userId, capabilityText);
        if (!existing.has_value()) {
            result = fail(422, "请填写 API Key");
            return false;
        }
        if (existing->provider != provider) {
            // 不同厂商的 Key 不可混用；更换厂商必须显式提供新 Key
            result = fail(422, "更换厂商后请重新填写 API Key");
            return false;
        }
        try {
            apiKey = SecretBox::open(existing->apiKeyEnc, userId, capabilityText,
                                     ConfigManager::encryptionKey);
        } catch (const std::exception &) {
            result = fail(409, "已保存的 API Key 无法解密（服务端加密密钥可能已变更），"
                               "请重新填写 API Key");
            return false;
        }
    }

    const std::string apiBase = AiCredentialRules::normalizeApiBase(stringField(body, "api_base"));
    const std::string model = stringField(body, "model");

    const std::string resolvedBase =
        apiBase.empty() ? AiCredentialRules::normalizeApiBase(preset->apiBase) : apiBase;
    const std::string resolvedModel =
        model.empty() ? AiCredentialRules::defaultModel(capability, provider) : model;

    std::string error;
    if (!AiCredentialRules::validateConfig(capability, provider, apiKey,
                                           resolvedBase, resolvedModel, error)) {
        result = fail(422, error);
        return false;
    }

    out.capability = AiCredentialRules::capabilityName(capability);
    out.provider = provider;
    out.apiKey = apiKey;
    out.apiBase = resolvedBase;
    out.model = resolvedModel;
    return true;
}

ServiceResult UserAiCredentialService::save(const std::string &userId,
                                           const std::string &capabilityText,
                                           const json &body) {
    if (!ConfigManager::byokEnabled) {
        return fail(404, "接口不存在");
    }
    if (!encryptionReady()) {
        return fail(503, kEncryptionMissing);
    }

    PreparedCredential prepared;
    ServiceResult result;
    if (!prepare(userId, capabilityText, body, prepared, result)) {
        return result;
    }

    const std::string verificationToken = stringField(body, "verification_token");
    if (verificationToken.empty()) {
        result = fail(422, "请先测试连接，验证通过后再保存");
        result.errorCode = "BYOK_VERIFICATION_REQUIRED";
        return result;
    }

    AiCredentialTicketContext ticketContext;
    ticketContext.userId = userId;
    ticketContext.capability = prepared.capability;
    ticketContext.provider = prepared.provider;
    ticketContext.apiBase = prepared.apiBase;
    ticketContext.model = prepared.model;
    ticketContext.apiKeyHash = EncryptionUtils::sha256(prepared.apiKey);

    std::string ticketError;
    if (!AiCredentialTicket::verify(ConfigManager::jwtSecret, verificationToken,
                                    ticketContext, ticketError)) {
        result = fail(422, ticketError);
        result.errorCode = "BYOK_VERIFICATION_REQUIRED";
        return result;
    }

    UserAiCredential credential;
    credential.userId = userId;
    credential.capability = prepared.capability;
    credential.provider = prepared.provider;
    credential.apiKeyHint = secretHint(prepared.apiKey);
    credential.apiBase = prepared.apiBase;
    credential.model = prepared.model;

    try {
        credential.apiKeyEnc = SecretBox::seal(
            userId, prepared.capability, prepared.apiKey, ConfigManager::encryptionKey);
    } catch (const std::exception &) {
        // 不记录明文 Key，也不把底层异常细节回传给前端
        spdlog::error("加密用户 AI 凭据失败: capability={}", prepared.capability);
        return fail(503, kEncryptionMissing);
    }

    if (!UserAiCredentialRepository::upsert(credential)) {
        return fail(500, "保存 API Key 失败，请稍后重试");
    }

    spdlog::info("用户 AI 凭据已更新: user={} capability={} provider={}",
                 userId, prepared.capability, prepared.provider);
    return ok("API Key 已保存", credential.toPublicJson());
}

ServiceResult UserAiCredentialService::remove(const std::string &userId,
                                             const std::string &capabilityText) {
    if (!ConfigManager::byokEnabled) {
        return fail(404, "接口不存在");
    }
    AiCapability capability = AiCapability::Unknown;
    if (!AiCredentialRules::parseCapability(capabilityText, capability)) {
        return fail(422, "capability 仅支持 llm 或 image");
    }

    const auto removed = UserAiCredentialRepository::remove(userId, capabilityText);
    if (removed == UserAiCredentialRepository::RemoveResult::Error) {
        return fail(500, "删除 API Key 失败，请稍后重试");
    }
    if (removed == UserAiCredentialRepository::RemoveResult::NotFound) {
        return fail(404, "未找到该能力的 API Key 配置");
    }

    spdlog::info("用户 AI 凭据已移除: user={} capability={}", userId, capabilityText);
    return ok("API Key 已移除", nullptr);
}

ServiceResult UserAiCredentialService::test(const std::string &userId, const json &body) {
    if (!ConfigManager::byokEnabled) {
        return fail(404, "接口不存在");
    }
    if (!encryptionReady()) {
        return fail(503, kEncryptionMissing);
    }

    const std::string capabilityText = stringField(body, "capability");
    PreparedCredential prepared;
    ServiceResult result;
    if (!prepare(userId, capabilityText, body, prepared, result)) {
        return result;
    }

    AiCredentialTestResult testResult;
    std::string transportError;
    const bool reachable = AiServiceClient::testCredential(
        prepared.capability, prepared.provider, prepared.apiKey,
        prepared.apiBase, prepared.model, testResult, transportError);

    if (!reachable) {
        // 业务结果仍返回 200，让前端统一展示测试失败原因；失败不允许保存
        return ok("测试连接未完成", {
            {"ok", false},
            {"verified", false},
            {"latency_ms", 0},
            {"message", transportError}
        });
    }

    json data = {
        {"ok", testResult.ok},
        {"verified", testResult.verified},
        {"latency_ms", testResult.latencyMs},
        {"message", testResult.message}
    };

    // 只有真实连通性验证通过才签发保存票据；票据仅绑定配置哈希与短期时间，
    // 不包含明文 Key，也不落库。
    if (testResult.ok && testResult.verified) {
        AiCredentialTicketContext ticketContext;
        ticketContext.userId = userId;
        ticketContext.capability = prepared.capability;
        ticketContext.provider = prepared.provider;
        ticketContext.apiBase = prepared.apiBase;
        ticketContext.model = prepared.model;
        ticketContext.apiKeyHash = EncryptionUtils::sha256(prepared.apiKey);
        try {
            data["verification_token"] = AiCredentialTicket::issue(
                ConfigManager::jwtSecret, ticketContext);
            data["expires_in"] = AiCredentialTicket::kDefaultTtlSeconds;
        } catch (const std::exception &) {
            return fail(500, "签发验证票据失败，请稍后重新测试");
        }
    }

    return ok("测试完成", data);
}

bool UserAiCredentialService::loadConfigured(const std::string &userId,
                                            bool &llmConfigured,
                                            bool &imageConfigured) {
    llmConfigured = false;
    imageConfigured = false;
    if (!ConfigManager::byokEnabled) {
        return true;
    }

    auto credentials = UserAiCredentialRepository::findByUser(userId);
    if (!credentials.has_value()) {
        return false;
    }
    for (const auto &credential : *credentials) {
        if (credential.capability == "llm") {
            llmConfigured = true;
        } else if (credential.capability == "image") {
            imageConfigured = true;
        }
    }
    return true;
}

ResolvedAiCredentials UserAiCredentialService::resolveForTask(const std::string &userId,
                                                             bool needLlm,
                                                             bool needImage) {
    ResolvedAiCredentials resolved;
    if (!needLlm && !needImage) {
        return resolved;
    }
    if (!ConfigManager::byokEnabled) {
        // 总开关关闭视为回滚：新任务不会打快照；已在队列中的旧快照按系统 Key 继续，
        // 符合「关闭后所有请求回到系统 Key」的运维语义。
        return resolved;
    }

    auto credentials = UserAiCredentialRepository::findByUser(userId);
    if (!credentials.has_value()) {
        resolved.ok = false;
        resolved.error = "查询你保存的 API Key 失败，请到个人中心检查配置后重试";
        return resolved;
    }

    for (const auto &credential : *credentials) {
        const bool isLlm = credential.capability == "llm";
        const bool isImage = credential.capability == "image";
        if (!isLlm && !isImage) {
            continue;
        }
        if (isLlm) resolved.llmConfigured = true;
        if (isImage) resolved.imageConfigured = true;

        if ((isLlm && !needLlm) || (isImage && !needImage)) {
            continue;
        }

        try {
            AiCredentialSecret secret;
            secret.capability = credential.capability;
            secret.provider = credential.provider;
            secret.apiKey = SecretBox::open(credential.apiKeyEnc, userId,
                                            credential.capability,
                                            ConfigManager::encryptionKey);
            secret.apiBase = credential.apiBase.value_or("");
            secret.model = credential.model.value_or("");
            if (isLlm) {
                resolved.llm = secret;
            } else {
                resolved.image = secret;
            }
        } catch (const std::exception &) {
            resolved.ok = false;
            const std::string label = AiCredentialRules::capabilityLabel(
                isLlm ? AiCapability::Llm : AiCapability::Image);
            resolved.error = "你保存的" + label + " API Key 已失效（服务端加密密钥可能已变更），"
                             "请到个人中心重新保存";
            return resolved;
        }
    }

    if (needLlm && !resolved.llm.has_value()) {
        resolved.ok = false;
        resolved.error = "该任务使用了你的文本理解 API Key，但配置已被移除；"
                        "请到个人中心重新保存后再试";
        return resolved;
    }
    if (needImage && !resolved.image.has_value()) {
        resolved.ok = false;
        resolved.error = "该任务使用了你的图像生成 API Key，但配置已被移除；"
                        "请到个人中心重新保存后再试";
        return resolved;
    }
    return resolved;
}
