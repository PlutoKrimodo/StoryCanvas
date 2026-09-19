#include "PasswordUtils.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <spdlog/spdlog.h>

#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Base64.h"

#if defined(SC_HAVE_BCRYPT)
#include <bcrypt/bcrypt.h>
#endif

namespace {

constexpr int kPbkdf2Iterations = 120000;
constexpr size_t kSaltBytes = 16;
constexpr size_t kKeyBytes = 32;

#if defined(SC_HAVE_BCRYPT)
constexpr int kBcryptCost = 12;
#endif

const std::string kPbkdf2Prefix = "pbkdf2_sha256";

bool looksLikeBcrypt(const std::string &hash) {
    return hash.compare(0, 4, "$2a$") == 0 ||
           hash.compare(0, 4, "$2b$") == 0 ||
           hash.compare(0, 4, "$2y$") == 0;
}

}  // namespace

std::string PasswordUtils::algorithmName() {
#if defined(SC_HAVE_BCRYPT)
    return "bcrypt";
#else
    return "pbkdf2_sha256";
#endif
}

bool PasswordUtils::constantTimeEquals(const std::string &lhs, const std::string &rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<unsigned char>(lhs[i] ^ rhs[i]);
    }
    return diff == 0;
}

std::string PasswordUtils::pbkdf2Hash(const std::string &password) {
    std::vector<unsigned char> salt(kSaltBytes);
    if (RAND_bytes(salt.data(), static_cast<int>(kSaltBytes)) != 1) {
        throw std::runtime_error("生成密码盐值失败");
    }

    std::vector<unsigned char> key(kKeyBytes);
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          kPbkdf2Iterations, EVP_sha256(),
                          static_cast<int>(kKeyBytes), key.data()) != 1) {
        throw std::runtime_error("PBKDF2 计算失败");
    }

    const std::string saltStr(reinterpret_cast<const char *>(salt.data()), salt.size());
    const std::string keyStr(reinterpret_cast<const char *>(key.data()), key.size());

    return kPbkdf2Prefix + "$" + std::to_string(kPbkdf2Iterations) + "$" +
           Base64::encode(saltStr) + "$" + Base64::encode(keyStr);
}

bool PasswordUtils::verifyPbkdf2(const std::string &password, const std::string &hash) {
    std::vector<std::string> parts;
    std::stringstream stream(hash);
    std::string token;
    while (std::getline(stream, token, '$')) {
        parts.push_back(token);
    }
    if (parts.size() != 4) {
        return false;
    }

    int iterations = 0;
    try {
        iterations = std::stoi(parts[1]);
    } catch (const std::exception &) {
        return false;
    }
    if (iterations <= 0) {
        return false;
    }

    std::string salt;
    std::string expected;
    try {
        salt = Base64::decode(parts[2]);
        expected = Base64::decode(parts[3]);
    } catch (const std::exception &) {
        return false;
    }
    if (salt.empty() || expected.empty()) {
        return false;
    }

    std::vector<unsigned char> key(expected.size());
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char *>(salt.data()),
                          static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          static_cast<int>(key.size()), key.data()) != 1) {
        return false;
    }

    const std::string actual(reinterpret_cast<const char *>(key.data()), key.size());
    return constantTimeEquals(actual, expected);
}

std::string PasswordUtils::hashPassword(const std::string &password) {
#if defined(SC_HAVE_BCRYPT)
    return bcryptHash(password);
#else
    return pbkdf2Hash(password);
#endif
}

bool PasswordUtils::verifyPassword(const std::string &password, const std::string &hash) {
    if (password.empty() || hash.empty()) {
        return false;
    }

    if (looksLikeBcrypt(hash)) {
#if defined(SC_HAVE_BCRYPT)
        return verifyBcrypt(password, hash);
#else
        spdlog::warn("检测到 bcrypt 哈希，但本次编译未启用 libbcrypt，无法校验");
        return false;
#endif
    }

    if (hash.compare(0, kPbkdf2Prefix.size(), kPbkdf2Prefix) == 0) {
        return verifyPbkdf2(password, hash);
    }

    spdlog::warn("无法识别的密码哈希格式");
    return false;
}

bool PasswordUtils::isStrongPassword(const std::string &password) {
    if (password.size() < 8 || password.size() > 128) {
        return false;
    }

    static const std::regex kUpper("[A-Z]");
    static const std::regex kLower("[a-z]");
    static const std::regex kDigit("[0-9]");

    return std::regex_search(password, kUpper) &&
           std::regex_search(password, kLower) &&
           std::regex_search(password, kDigit);
}

#if defined(SC_HAVE_BCRYPT)
std::string PasswordUtils::bcryptHash(const std::string &password) {
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];

    if (bcrypt_gensalt(kBcryptCost, salt) != 0) {
        throw std::runtime_error("bcrypt 生成盐值失败");
    }
    if (bcrypt_hashpw(password.c_str(), salt, hash) != 0) {
        throw std::runtime_error("bcrypt 计算失败");
    }
    return std::string(hash);
}

bool PasswordUtils::verifyBcrypt(const std::string &password, const std::string &hash) {
    return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
}
#endif
