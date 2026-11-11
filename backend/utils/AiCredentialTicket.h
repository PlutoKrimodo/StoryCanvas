#pragma once

#include <string>

/**
 * BYOK 凭据验证票据上下文。
 *
 * 票据只绑定“这次被验证的配置”，不包含明文 API Key；apiKeyHash 为
 * sha256(apiKey) 的十六进制字符串。
 */
struct AiCredentialTicketContext {
    std::string userId;
    std::string capability;
    std::string provider;
    std::string apiBase;
    std::string model;
    std::string apiKeyHash;
};

/**
 * 无状态、短期有效的 BYOK 验证票据。
 *
 * 实现：Base64URL(payloadJson) + "." + Base64URL(HMAC-SHA256(secret, payload))。
 * payload 包含 userId、capability、provider、apiBase、model、apiKeyHash、
 * 过期时间和随机 nonce；不保存明文 Key，也不新增数据库记录。
 */
class AiCredentialTicket {
public:
    static constexpr long long kDefaultTtlSeconds = 600;

    /** 签发票据；参数非法或密钥为空时抛 std::runtime_error。 */
    static std::string issue(const std::string &secret,
                             const AiCredentialTicketContext &context,
                             long long ttlSeconds = kDefaultTtlSeconds,
                             long long nowUnix = 0);

    /**
     * 校验票据签名、过期时间和配置绑定关系。
     * nowUnix=0 表示使用当前时间；测试可注入固定时间。
     */
    static bool verify(const std::string &secret,
                       const std::string &token,
                       const AiCredentialTicketContext &context,
                       std::string &error,
                       long long nowUnix = 0);
};
