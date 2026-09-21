#include <gtest/gtest.h>

#include <string>

#include "utils/EncryptionUtils.h"

TEST(EncryptionUtilsTest, AesRoundTripRestoresPlaintext) {
    const std::string key = EncryptionUtils::generateKey();
    const std::string plaintext = "secret-value-123";

    const std::string ciphertext = EncryptionUtils::encryptAES(plaintext, key);

    EXPECT_NE(ciphertext, plaintext);
    EXPECT_EQ(ciphertext.find(plaintext), std::string::npos);
    EXPECT_EQ(EncryptionUtils::decryptAES(ciphertext, key), plaintext);
}

/** 每次加密使用随机 IV，相同明文密文不同 */
TEST(EncryptionUtilsTest, AesUsesRandomIvPerCall) {
    const std::string key = EncryptionUtils::generateKey();

    EXPECT_NE(EncryptionUtils::encryptAES("same-text", key),
              EncryptionUtils::encryptAES("same-text", key));
}

/** GCM 认证标签：错误密钥必须解密失败而不是返回错误明文 */
TEST(EncryptionUtilsTest, AesRejectsWrongKey) {
    const std::string ciphertext =
        EncryptionUtils::encryptAES("payload", EncryptionUtils::generateKey());

    EXPECT_THROW(EncryptionUtils::decryptAES(ciphertext, EncryptionUtils::generateKey()),
                 std::runtime_error);
}

TEST(EncryptionUtilsTest, AesRejectsEmptyKey) {
    EXPECT_THROW(EncryptionUtils::encryptAES("payload", ""), std::runtime_error);
}

TEST(EncryptionUtilsTest, Sha256IsStableHexDigest) {
    const std::string digest = EncryptionUtils::sha256("storycanvas");

    EXPECT_EQ(digest.size(), 64u);
    EXPECT_EQ(digest, EncryptionUtils::sha256("storycanvas"));
    EXPECT_NE(digest, EncryptionUtils::sha256("storycanvas2"));
    EXPECT_EQ(digest.find_first_not_of("0123456789abcdef"), std::string::npos);
}

TEST(EncryptionUtilsTest, HmacDependsOnKeyAndMessage) {
    EXPECT_EQ(EncryptionUtils::hmacSha256("key-a", "msg"),
              EncryptionUtils::hmacSha256("key-a", "msg"));
    EXPECT_NE(EncryptionUtils::hmacSha256("key-a", "msg"),
              EncryptionUtils::hmacSha256("key-b", "msg"));
    EXPECT_NE(EncryptionUtils::hmacSha256("key-a", "msg"),
              EncryptionUtils::hmacSha256("key-a", "msg2"));
}

TEST(EncryptionUtilsTest, RandomHexHasExpectedLengthAndUniqueness) {
    EXPECT_EQ(EncryptionUtils::randomHex(16).size(), 32u);
    EXPECT_NE(EncryptionUtils::randomHex(16), EncryptionUtils::randomHex(16));
}

TEST(EncryptionUtilsTest, MaskSecretHidesMiddle) {
    EXPECT_EQ(EncryptionUtils::maskSecret(""), "");
    EXPECT_EQ(EncryptionUtils::maskSecret("short"), "****");
    EXPECT_EQ(EncryptionUtils::maskSecret("abcdefghijkl"), "abcd****ijkl");
}

TEST(EncryptionUtilsTest, ConstantTimeEqualsMatchesSemantics) {
    EXPECT_TRUE(EncryptionUtils::constantTimeEquals("abc", "abc"));
    EXPECT_FALSE(EncryptionUtils::constantTimeEquals("abc", "abd"));
    EXPECT_FALSE(EncryptionUtils::constantTimeEquals("abc", "abcd"));
}
