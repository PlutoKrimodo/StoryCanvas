#include <gtest/gtest.h>

#include <string>

#include "utils/PasswordUtils.h"

namespace {

bool looksLikeBcrypt(const std::string &hash) {
    return hash.rfind("$2a$", 0) == 0 ||
           hash.rfind("$2b$", 0) == 0 ||
           hash.rfind("$2y$", 0) == 0;
}

bool looksLikePbkdf2(const std::string &hash) {
    return hash.rfind("pbkdf2_sha256$", 0) == 0;
}

}  // namespace

/** 密码必须单向哈希存储，绝不允许明文 */
TEST(PasswordUtilsTest, HashIsNeverPlaintext) {
    const std::string password = "StrongPass1!";
    const std::string hash = PasswordUtils::hashPassword(password);

    EXPECT_FALSE(hash.empty());
    EXPECT_NE(hash, password);
    EXPECT_EQ(hash.find(password), std::string::npos);
}

/** 哈希串自描述，便于算法平滑升级 */
TEST(PasswordUtilsTest, HashFormatIsSelfDescribing) {
    const std::string hash = PasswordUtils::hashPassword("StrongPass1!");
    EXPECT_TRUE(looksLikeBcrypt(hash) || looksLikePbkdf2(hash))
        << "未识别的哈希格式: " << hash.substr(0, 12);
}

TEST(PasswordUtilsTest, AlgorithmNameMatchesImplementation) {
    const std::string name = PasswordUtils::algorithmName();
    EXPECT_TRUE(name == "bcrypt" || name == "pbkdf2_sha256");
}

/** 加盐存储：同一密码两次哈希结果必须不同 */
TEST(PasswordUtilsTest, SamePasswordProducesDifferentHashes) {
    const std::string password = "StrongPass1!";
    EXPECT_NE(PasswordUtils::hashPassword(password),
              PasswordUtils::hashPassword(password));
}

TEST(PasswordUtilsTest, VerifyAcceptsCorrectPassword) {
    const std::string password = "StrongPass1!";
    const std::string hash = PasswordUtils::hashPassword(password);

    EXPECT_TRUE(PasswordUtils::verifyPassword(password, hash));
}

TEST(PasswordUtilsTest, VerifyRejectsWrongOrEmptyInput) {
    const std::string password = "StrongPass1!";
    const std::string hash = PasswordUtils::hashPassword(password);

    EXPECT_FALSE(PasswordUtils::verifyPassword("WrongPass1!", hash));
    EXPECT_FALSE(PasswordUtils::verifyPassword("", hash));
    EXPECT_FALSE(PasswordUtils::verifyPassword(password, ""));
}

TEST(PasswordUtilsTest, VerifyRejectsMalformedHash) {
    EXPECT_FALSE(PasswordUtils::verifyPassword("StrongPass1!", "not-a-valid-hash"));
    EXPECT_FALSE(PasswordUtils::verifyPassword("StrongPass1!", "pbkdf2_sha256$0$$"));
}

/** 密码强度策略：≥8 位，且同时包含大小写字母与数字 */
TEST(PasswordUtilsTest, PasswordStrengthPolicy) {
    EXPECT_TRUE(PasswordUtils::isStrongPassword("StrongPass1"));
    EXPECT_TRUE(PasswordUtils::isStrongPassword("Abcdefg1"));

    EXPECT_FALSE(PasswordUtils::isStrongPassword("weak"));
    EXPECT_FALSE(PasswordUtils::isStrongPassword("Ab1"));
    EXPECT_FALSE(PasswordUtils::isStrongPassword("NOLOWERCASE1"));
    EXPECT_FALSE(PasswordUtils::isStrongPassword("nouppercase1"));
    EXPECT_FALSE(PasswordUtils::isStrongPassword("NoDigitsHere"));
    EXPECT_FALSE(PasswordUtils::isStrongPassword(std::string(129, 'A') + "a1"));
}
