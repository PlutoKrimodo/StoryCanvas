#pragma once

#include <string>

/**
 * 加密与摘要工具（加密存储基础设施）
 *
 * - encryptAES / decryptAES：AES-256-GCM，密文格式为 Base64(IV | CipherText | Tag)
 * - generateKey：生成 32 字节随机密钥（Base64 字符串，用于 ENCRYPTION_KEY 配置）
 * - sha256：十六进制摘要，用于 Refresh Token 等不可逆存储
 * - hmacSha256：HMAC-SHA256 原始字节，用于 JWT HS256 签名
 * - randomHex：生成随机十六进制串（盐值、文件名、jti）
 * - maskSecret：日志脱敏，仅保留首尾少量字符
 *
 * 约束：密钥一律通过环境变量注入，禁止硬编码与提交入库。
 */
class EncryptionUtils {
public:
    static std::string encryptAES(const std::string &plaintext, const std::string &key);
    static std::string decryptAES(const std::string &ciphertext, const std::string &key);

    static std::string generateKey();

    static std::string sha256(const std::string &input);
    static std::string hmacSha256(const std::string &key, const std::string &message);

    static std::string randomHex(size_t bytes);
    static std::string maskSecret(const std::string &secret);

    /** 恒定时间比较，用于签名/哈希比对，避免时序侧信道 */
    static bool constantTimeEquals(const std::string &lhs, const std::string &rhs);
};
