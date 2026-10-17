#pragma once

#include <string>

/**
 * BYOK 凭据信封加密。
 *
 * 在明文前附加 `userId + 0x1F + capability + 0x1F` 前缀后再做 AES-256-GCM 加密，
 * 解密时以恒定时间比对该前缀。效果：
 *   - 将 A 用户的密文改到 B 用户 / 另一项能力下，解密后前缀不匹配，直接拒绝；
 *   - GCM 认证标签同时保证密文未被篡改。
 *
 * 仅依赖 EncryptionUtils（OpenSSL），不依赖 Drogon，便于独立单测。
 */
class SecretBox {
public:
    /** 加密并返回 Base64(IV|Cipher|Tag) 形式的密文；失败抛 std::runtime_error。 */
    static std::string seal(const std::string &userId,
                            const std::string &capability,
                            const std::string &plaintext,
                            const std::string &key);

    /** 解密并校验绑定前缀；不匹配或密文损坏时抛 std::runtime_error。 */
    static std::string open(const std::string &ciphertext,
                            const std::string &userId,
                            const std::string &capability,
                            const std::string &key);

    /** 脱敏展示（sk-****abcd），与 EncryptionUtils::maskSecret 口径一致。 */
    static std::string hint(const std::string &secret);

    /** 加密密钥是否可用：非空即视为已配置（长度/派生由 EncryptionUtils 负责）。 */
    static bool hasKey(const std::string &key);
};
