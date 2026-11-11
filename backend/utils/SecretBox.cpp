#include "SecretBox.h"

#include <stdexcept>

#include "EncryptionUtils.h"

namespace {

constexpr char kSeparator = '\x1F';

std::string buildEnvelope(const std::string &userId,
                          const std::string &capability,
                          const std::string &plaintext) {
    if (userId.empty()) {
        throw std::runtime_error("凭据缺少用户绑定信息");
    }
    if (capability.empty()) {
        throw std::runtime_error("凭据缺少能力类型");
    }
    if (plaintext.empty()) {
        throw std::runtime_error("凭据内容为空");
    }
    std::string envelope;
    envelope.reserve(userId.size() + capability.size() + plaintext.size() + 2);
    envelope.append(userId);
    envelope.push_back(kSeparator);
    envelope.append(capability);
    envelope.push_back(kSeparator);
    envelope.append(plaintext);
    return envelope;
}

/**
 * 拆分信封并校验前缀。
 *
 * 使用恒定时间比较，避免通过响应耗时侧信道推断其他用户 ID / 能力字符串。
 */
std::string unwrapEnvelope(const std::string &envelope,
                           const std::string &userId,
                           const std::string &capability) {
    const std::size_t first = envelope.find(kSeparator);
    if (first == std::string::npos) {
        throw std::runtime_error("凭据密文格式非法");
    }
    const std::size_t second = envelope.find(kSeparator, first + 1);
    if (second == std::string::npos) {
        throw std::runtime_error("凭据密文格式非法");
    }

    const std::string boundUser = envelope.substr(0, first);
    const std::string boundCapability = envelope.substr(first + 1, second - first - 1);
    if (!EncryptionUtils::constantTimeEquals(boundUser, userId) ||
        !EncryptionUtils::constantTimeEquals(boundCapability, capability)) {
        // 不把期望值/实际值写进异常，避免日志泄漏用户 ID 或密文内容
        throw std::runtime_error("凭据与当前用户或能力不匹配");
    }

    const std::string plaintext = envelope.substr(second + 1);
    if (plaintext.empty()) {
        throw std::runtime_error("凭据内容为空");
    }
    return plaintext;
}

}  // namespace

std::string SecretBox::seal(const std::string &userId,
                            const std::string &capability,
                            const std::string &plaintext,
                            const std::string &key) {
    if (!hasKey(key)) {
        throw std::runtime_error("ENCRYPTION_KEY 未配置，无法加密保存 API Key");
    }
    return EncryptionUtils::encryptAES(buildEnvelope(userId, capability, plaintext), key);
}

std::string SecretBox::open(const std::string &ciphertext,
                            const std::string &userId,
                            const std::string &capability,
                            const std::string &key) {
    if (!hasKey(key)) {
        throw std::runtime_error("ENCRYPTION_KEY 未配置，无法读取 API Key");
    }
    if (ciphertext.empty()) {
        throw std::runtime_error("凭据密文为空");
    }
    const std::string envelope = EncryptionUtils::decryptAES(ciphertext, key);
    return unwrapEnvelope(envelope, userId, capability);
}

std::string SecretBox::hint(const std::string &secret) {
    return EncryptionUtils::maskSecret(secret);
}

bool SecretBox::hasKey(const std::string &key) {
    std::string error;
    return EncryptionUtils::isUsableKey(key, error);
}
