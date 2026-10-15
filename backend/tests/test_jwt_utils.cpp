#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "utils/JwtUtils.h"

namespace {

constexpr char kSecret[] = "unit-test-secret-not-for-production";
constexpr int kAccessExpiry = 3600;
constexpr int kRefreshExpiry = 604800;

/** 与 JwtUtils 内部一致的浮点秒时钟，用于断言撤销边界 */
double nowSeconds() {
    return std::chrono::duration<double>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

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

    const double issuedBefore = nowSeconds();  // 撤销前的时刻（浮点秒）
    JwtUtils::revokeAllForUser("user-pwd");

    EXPECT_FALSE(JwtUtils::verifyToken(tokens.accessToken, "access").has_value());
    EXPECT_FALSE(JwtUtils::verifyToken(tokens.refreshToken, "refresh").has_value());
    // 撤销时刻之前签发的时间戳一律失效；之后签发的（如改密后重登新 Token）见回归用例
    EXPECT_TRUE(JwtUtils::isUserTokenRevoked("user-pwd", issuedBefore));
}

/**
 * 回归（2026-10-15）：改密与重新登录落在同一秒（脚本 / 自动化常见）时，
 * 新 Token 不得被整用户撤销误伤——iat 与撤销时刻均为毫秒级浮点秒。
 */
TEST_F(JwtUtilsTest, FreshLoginRightAfterRevokeAllIsNotRevoked) {
    JwtUtils::revokeAllForUser("user-relogin");
    // 模拟改密与重登之间的微小间隔（旧实现的秒级粒度会吃掉整个同一秒）
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    const TokenPair fresh = JwtUtils::generateTokenPair("user-relogin", "alice");
    const auto payload = JwtUtils::verifyToken(fresh.accessToken, "access");

    ASSERT_TRUE(payload.has_value());
    EXPECT_FALSE(JwtUtils::isUserTokenRevoked("user-relogin", payload->issuedAt));
    // 安全语义不变：撤销时刻之前签发的 Token 仍然失效
    EXPECT_TRUE(JwtUtils::isUserTokenRevoked("user-relogin", nowSeconds() - 1.0));
}

TEST_F(JwtUtilsTest, GuestTokenIsAccessTypeAndMarkedGuest) {
    const std::string token = JwtUtils::generateGuestToken("guest-1", "guest_abc");
    const auto payload = JwtUtils::verifyToken(token, "access");

    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->userId, "guest-1");
    EXPECT_TRUE(payload->isGuest);
    // 游客 Token 复用 access 类型，便于既有 JwtFilter 直接放行
    EXPECT_EQ(payload->type, "access");
    EXPECT_FALSE(JwtUtils::verifyToken(token, "refresh").has_value());
}

TEST_F(JwtUtilsTest, RegularAccessTokenIsNotGuest) {
    const TokenPair tokens = JwtUtils::generateTokenPair("user-1", "alice");
    const auto payload = JwtUtils::verifyToken(tokens.accessToken, "access");

    ASSERT_TRUE(payload.has_value());
    EXPECT_FALSE(payload->isGuest);
}

TEST_F(JwtUtilsTest, RevokeAllForUserDoesNotAffectOtherUsers) {
    JwtUtils::revokeAllForUser("user-a");

    const TokenPair other = JwtUtils::generateTokenPair("user-b", "bob");
    EXPECT_TRUE(JwtUtils::verifyToken(other.accessToken, "access").has_value());
    EXPECT_FALSE(JwtUtils::isUserTokenRevoked("user-b", nowSeconds()));
}
