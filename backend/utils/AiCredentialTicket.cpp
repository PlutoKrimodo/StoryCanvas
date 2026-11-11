#include "AiCredentialTicket.h"

#include <chrono>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "Base64.h"
#include "EncryptionUtils.h"

namespace {

using json = nlohmann::json;

constexpr std::size_t kMaxTokenLength = 4096;

long long currentUnix() {
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

long long effectiveNow(long long nowUnix) {
    return nowUnix > 0 ? nowUnix : currentUnix();
}

std::string requiredField(const json &body, const char *key) {
    if (!body.is_object() || !body.contains(key) || !body[key].is_string()) {
        throw std::runtime_error("验证票据字段缺失");
    }
    return body[key].get<std::string>();
}

}  // namespace

std::string AiCredentialTicket::issue(const std::string &secret,
                                      const AiCredentialTicketContext &context,
                                      long long ttlSeconds,
                                      long long nowUnix) {
    if (secret.empty()) {
        throw std::runtime_error("验证票据签名密钥未配置");
    }
    if (context.userId.empty() || context.capability.empty() ||
        context.provider.empty() || context.apiKeyHash.empty()) {
        throw std::runtime_error("验证票据缺少必要绑定字段");
    }

    const long long issuedAt = effectiveNow(nowUnix);
    const long long ttl = (ttlSeconds > 0 && ttlSeconds <= 3600)
                              ? ttlSeconds
                              : kDefaultTtlSeconds;

    const json payload = {
        {"sub", context.userId},
        {"cap", context.capability},
        {"prov", context.provider},
        {"base", context.apiBase},
        {"model", context.model},
        {"key", context.apiKeyHash},
        {"exp", issuedAt + ttl},
        {"nonce", EncryptionUtils::randomHex(16)},
    };

    const std::string encodedPayload = Base64::encodeUrl(payload.dump());
    const std::string signature =
        Base64::encodeUrl(EncryptionUtils::hmacSha256(secret, encodedPayload));
    return encodedPayload + "." + signature;
}

bool AiCredentialTicket::verify(const std::string &secret,
                                const std::string &token,
                                const AiCredentialTicketContext &context,
                                std::string &error,
                                long long nowUnix) {
    error.clear();
    if (secret.empty()) {
        error = "验证票据签名密钥未配置";
        return false;
    }
    if (token.empty() || token.size() > kMaxTokenLength) {
        error = "验证票据格式非法";
        return false;
    }

    const std::size_t split = token.rfind('.');
    if (split == std::string::npos || split == 0 || split + 1 >= token.size()) {
        error = "验证票据格式非法";
        return false;
    }

    const std::string encodedPayload = token.substr(0, split);
    const std::string encodedSignature = token.substr(split + 1);

    try {
        const std::string actualSignature = Base64::decodeUrl(encodedSignature);
        const std::string expectedSignature =
            EncryptionUtils::hmacSha256(secret, encodedPayload);
        if (!EncryptionUtils::constantTimeEquals(actualSignature, expectedSignature)) {
            error = "验证票据签名不匹配，请重新测试连接";
            return false;
        }

        const json payload = json::parse(Base64::decodeUrl(encodedPayload));
        if (!payload.is_object()) {
            error = "验证票据内容非法";
            return false;
        }

        const std::string userId = requiredField(payload, "sub");
        const std::string capability = requiredField(payload, "cap");
        const std::string provider = requiredField(payload, "prov");
        const std::string apiBase = requiredField(payload, "base");
        const std::string model = requiredField(payload, "model");
        const std::string apiKeyHash = requiredField(payload, "key");
        if (!payload.contains("exp") || !payload["exp"].is_number_integer()) {
            error = "验证票据缺少有效期";
            return false;
        }
        const long long expiresAt = payload["exp"].get<long long>();
        if (effectiveNow(nowUnix) >= expiresAt) {
            error = "验证票据已过期，请重新测试连接";
            return false;
        }

        if (userId != context.userId || capability != context.capability ||
            provider != context.provider || apiBase != context.apiBase ||
            model != context.model ||
            !EncryptionUtils::constantTimeEquals(apiKeyHash, context.apiKeyHash)) {
            error = "验证票据与当前配置不匹配，请重新测试连接";
            return false;
        }

        return true;
    } catch (const std::exception &) {
        error = "验证票据解析失败，请重新测试连接";
        return false;
    }
}
