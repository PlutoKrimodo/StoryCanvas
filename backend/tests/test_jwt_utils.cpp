#include <gtest/gtest.h>

#include <ctime>
#include <stdexcept>
#include <string>

#include "utils/JwtUtils.h"

namespace {

constexpr char kSecret[] = "unit-test-secret-not-for-production";
constexpr int kAccessExpiry = 3600;
constexpr int kRefreshExpiry = 604800;

class JwtUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清除跨用例的全局撤销状态，保证用例相互独立
        JwtUtils::resetRevocations();
        JwtUtils::init(kSecret, kAccessExpiry, kRefreshExpiry);
    }

    void TearDown() override {
        JwtUtils::resetRevocations();
    }
};

}  // namespace

TEST_F(JwtUtilsTest, GeneratesDistinctAccessAndRefreshTokens) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-1", "alice");

    EXPECT_FALSE(tokens.accessToken.empty());
    EXPECT_FALSE(tokens.refreshToken.empty());
    EXPECT_NE(tokens.accessToken, tokens.refreshToken);
}

TEST_F(JwtUtilsTest, AccessTokenCarriesIdentityAndExpiry) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-1", "alice");
    const auto payload = JwtUtils::verifyToken(tokens.accessToken, "access");

    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->userId, "user-1");
    EXPECT_EQ(payload->username, "alice");
    EXPECT_EQ(payload->type, "access");
    EXPECT_FALSE(payload->jti.empty());
    EXPECT_GT(payload->expiresAt, payload->issuedAt);
}

TEST_F(JwtUtilsTest, TokenTypeMismatchIsRejected) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-1", "alice");

    EXPECT_FALSE(JwtUtils::verifyToken(tokens.accessToken, "refresh").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken(tokens.refreshToken, "access").has_value());
}

TEST_F(JwtUtilsTest, TamperedTokenIsRejected) {
    std::string token = JwtUtils::generateTokenPair("user-1", "alice").accessToken;
    token.back() = (token.back() == 'A') ? 'B' : 'A';

    EXPECT_FALSE(JwtUtils::verifyToken(token, "access").has_value());
}

TEST_F(JwtUtilsTest, MalformedTokenIsRejected) {
    EXPECT_FALSE(JwtUtils::verifyToken("", "access").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken("only-one-segment", "access").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken("not.a.jwt", "access").has_value());
}

TEST_F(JwtUtilsTest, ExpiredTokenIsRejected) {
    JwtUtils::init(kSecret, -10, -10);  // 签发即过期
    const TokenPair tokens = JwtUtils::generateTokenPair("user-exp", "exp");

    EXPECT_FALSE(JwtUtils::verifyToken(tokens.accessToken, "access").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken(tokens.refreshToken, "refresh").has_value());
}

TEST_F(JwtUtilsTest, EmptySecretIsRejectedOnInit) {
    EXPECT_THROW(JwtUtils::init("", kAccessExpiry, kRefreshExpiry), std::runtime_error);
}

TEST_F(JwtUtilsTest, RevokeByJtiInvalidatesSingleToken) {
    const std::string token = JwtUtils::generateTokenPair("user-1", "alice").accessToken;
    ASSERT_TRUE(JwtUtils::verifyToken(token, "access").has_value());

    JwtUtils::revokeToken(token);
    EXPECT_FALSE(JwtUtils::verifyToken(token, "access").has_value());
}

TEST_F(JwtUtilsTest, RefreshRotatesAndInvalidatesOldRefreshToken) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-1", "alice");
    const auto refreshed = JwtUtils::refresh(tokens.refreshToken);

    ASSERT_TRUE(refreshed.has_value());
    EXPECT_FALSE(refreshed->accessToken.empty());
    EXPECT_FALSE(refreshed->refreshToken.empty());

    // 轮换后旧 Refresh Token 立即作废
    EXPECT_FALSE(JwtUtils::verifyToken(tokens.refreshToken, "refresh").has_value());
    EXPECT_TRUE(JwtUtils::verifyToken(refreshed->refreshToken, "refresh").has_value());
}

/** Phase 2 核心安全项：修改密码后该用户全部历史 Token 失效 */
TEST_F(JwtUtilsTest, RevokeAllForUserInvalidatesEveryTokenOfThatUser) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-pwd", "alice");
    ASSERT_TRUE(JwtUtils::verifyToken(tokens.accessToken, "access").has_value());

    JwtUtils::revokeAllForUser("user-pwd");

    EXPECT_FALSE(JwtUtils::verifyToken(tokens.accessToken, "access").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken(tokens.refreshToken, "refresh").has_value());
    EXPECT_TRUE(JwtUtils::isUserTokenRevoked("user-pwd", std::time(nullptr)));
}

TEST_F(JwtUtilsTest, RevokeAllForUserDoesNotAffectOtherUsers) {
    JwtUtils::revokeAllForUser("user-a");

    const TokenPair other = JwtUtils::generateTokenPair("user-b", "bob");
    EXPECT_TRUE(JwtUtils::verifyToken(other.accessToken, "access").has_value());
    EXPECT_FALSE(JwtUtils::isUserTokenRevoked("user-b", std::time(nullptr)));
}
