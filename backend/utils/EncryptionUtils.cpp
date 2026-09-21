#include "EncryptionUtils.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <stdexcept>
#include <vector>

#include "Base64.h"

namespace {

constexpr int kIvLength = 12;   // GCM 推荐 96 bit
constexpr int kTagLength = 16;  // GCM 认证标签 128 bit
constexpr int kKeyLength = 32;  // AES-256

/**
 * 归一化密钥：优先按 Base64 解码为 32 字节；否则对原串做 SHA-256 派生。
 */
std::string deriveKey(const std::string &key) {
    if (key.empty()) {
        throw std::runtime_error("加密密钥为空，请通过 ENCRYPTION_KEY 环境变量注入");
    }

    try {
        const std::string decoded = Base64::decode(key);
        if (decoded.size() == kKeyLength) {
            return decoded;
        }
    } catch (const std::exception &) {
        // 不是合法 Base64，走派生分支
    }

    std::string digest(kKeyLength, '\0');
    unsigned int digestLen = 0;
    if (EVP_Digest(key.data(), key.size(),
                   reinterpret_cast<unsigned char *>(&digest[0]), &digestLen,
                   EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("派生加密密钥失败");
    }
    digest.resize(digestLen);
    return digest;
}

void assertKeyLength(const std::string &key) {
    if (key.size() != kKeyLength) {
        throw std::runtime_error("AES 密钥长度必须为 32 字节");
    }
}

}  // namespace

std::string EncryptionUtils::encryptAES(const std::string &plaintext, const std::string &key) {
    const std::string keyBytes = deriveKey(key);
    assertKeyLength(keyBytes);

    std::vector<unsigned char> iv(kIvLength);
    if (RAND_bytes(iv.data(), kIvLength) != 1) {
        throw std::runtime_error("生成随机 IV 失败");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("创建加密上下文失败");
    }

    std::string cipherBuf(plaintext.size() + kTagLength, '\0');
    int written = 0;
    int total = 0;
    std::vector<unsigned char> tag(kTagLength);

    const bool ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIvLength, nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char *>(keyBytes.data()), iv.data()) == 1 &&
        EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char *>(&cipherBuf[0]), &written,
                          reinterpret_cast<const unsigned char *>(plaintext.data()),
                          static_cast<int>(plaintext.size())) == 1;
    if (!ok) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES 加密初始化失败");
    }
    total = written;

    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&cipherBuf[0]) + total, &written) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLength, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES 加密失败");
    }
    total += written;
    EVP_CIPHER_CTX_free(ctx);

    std::string blob(reinterpret_cast<const char *>(iv.data()), kIvLength);
    blob.append(cipherBuf.data(), static_cast<size_t>(total));
    blob.append(reinterpret_cast<const char *>(tag.data()), kTagLength);
    return Base64::encode(blob);
}

std::string EncryptionUtils::decryptAES(const std::string &ciphertext, const std::string &key) {
    const std::string keyBytes = deriveKey(key);
    assertKeyLength(keyBytes);

    const std::string blob = Base64::decode(ciphertext);
    if (blob.size() < static_cast<size_t>(kIvLength + kTagLength)) {
        throw std::runtime_error("AES 密文长度非法");
    }

    const size_t contentLen = blob.size() - kIvLength - kTagLength;
    const unsigned char *iv = reinterpret_cast<const unsigned char *>(blob.data());
    const unsigned char *content = iv + kIvLength;
    const unsigned char *tag = content + contentLen;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("创建解密上下文失败");
    }

    std::string plainBuf(contentLen, '\0');
    int written = 0;
    int total = 0;

    const bool ok =
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIvLength, nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char *>(keyBytes.data()), iv) == 1 &&
        EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char *>(&plainBuf[0]), &written,
                          content, static_cast<int>(contentLen)) == 1;
    if (!ok) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES 解密初始化失败");
    }
    total = written;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagLength,
                            const_cast<unsigned char *>(tag)) != 1 ||
        EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&plainBuf[0]) + total, &written) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES 解密失败：认证标签校验不通过");
    }
    total += written;
    EVP_CIPHER_CTX_free(ctx);

    plainBuf.resize(static_cast<size_t>(total));
    return plainBuf;
}

std::string EncryptionUtils::generateKey() {
    std::vector<unsigned char> key(kKeyLength);
    if (RAND_bytes(key.data(), kKeyLength) != 1) {
        throw std::runtime_error("生成随机密钥失败");
    }
    return Base64::encode(std::string(reinterpret_cast<const char *>(key.data()), kKeyLength));
}

std::string EncryptionUtils::sha256(const std::string &input) {
    std::string digest(kKeyLength, '\0');
    unsigned int len = 0;
    if (EVP_Digest(input.data(), input.size(),
                   reinterpret_cast<unsigned char *>(&digest[0]), &len,
                   EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("SHA-256 计算失败");
    }
    digest.resize(len);

    static const char *kHex = "0123456789abcdef";
    std::string hex;
    hex.reserve(digest.size() * 2);
    for (unsigned char ch : digest) {
        hex.push_back(kHex[ch >> 4]);
        hex.push_back(kHex[ch & 0x0F]);
    }
    return hex;
}

std::string EncryptionUtils::hmacSha256(const std::string &key, const std::string &message) {
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_HMAC, nullptr,
        reinterpret_cast<const unsigned char *>(key.data()), key.size());
    if (pkey == nullptr) {
        throw std::runtime_error("创建 HMAC 密钥失败");
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("创建 HMAC 上下文失败");
    }

    size_t sigLen = 0;
    bool ok = EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) == 1 &&
              EVP_DigestSignUpdate(ctx, message.data(), message.size()) == 1 &&
              EVP_DigestSignFinal(ctx, nullptr, &sigLen) == 1;
    if (!ok) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("HMAC-SHA256 计算失败");
    }

    std::string signature(sigLen, '\0');
    ok = EVP_DigestSignFinal(ctx, reinterpret_cast<unsigned char *>(&signature[0]), &sigLen) == 1;

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (!ok) {
        throw std::runtime_error("HMAC-SHA256 计算失败");
    }
    signature.resize(sigLen);
    return signature;
}

std::string EncryptionUtils::randomHex(size_t bytes) {
    std::vector<unsigned char> buffer(bytes);
    if (RAND_bytes(buffer.data(), static_cast<int>(bytes)) != 1) {
        throw std::runtime_error("生成随机数失败");
    }

    static const char *kHex = "0123456789abcdef";
    std::string hex;
    hex.reserve(bytes * 2);
    for (unsigned char ch : buffer) {
        hex.push_back(kHex[ch >> 4]);
        hex.push_back(kHex[ch & 0x0F]);
    }
    return hex;
}

bool EncryptionUtils::constantTimeEquals(const std::string &lhs, const std::string &rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<unsigned char>(lhs[i] ^ rhs[i]);
    }
    return diff == 0;
}

std::string EncryptionUtils::maskSecret(const std::string &secret) {
    if (secret.empty()) {
        return "";
    }
    if (secret.size() <= 8) {
        return "****";
    }
    return secret.substr(0, 4) + "****" + secret.substr(secret.size() - 4);
}
