#include <gtest/gtest.h>

#include <string>

#include "utils/RateLimiter.h"

namespace {

constexpr std::time_t kBase = 1'800'000'000;  // 固定时间基准，避免依赖真实时钟
constexpr int kWindow = 60;

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override { RateLimiter::reset(); }
    void TearDown() override { RateLimiter::reset(); }
};

}  // namespace

TEST_F(RateLimiterTest, AllowsUpToLimitWithinWindow) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(RateLimiter::allowAt("ip:1.2.3.4", 5, kWindow, kBase)) << "第 " << i + 1 << " 次应放行";
    }
    // 第 6 次（同窗口内）拒绝
    EXPECT_FALSE(RateLimiter::allowAt("ip:1.2.3.4", 5, kWindow, kBase));
}

TEST_F(RateLimiterTest, ResetsCounterInNextWindow) {
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase));
    }
    EXPECT_FALSE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase));

    // 窗口到期后计数重置
    EXPECT_TRUE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase + kWindow));
    EXPECT_TRUE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase + kWindow));
    EXPECT_TRUE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase + kWindow));
    EXPECT_FALSE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase + kWindow));

    // 窗口边界前一秒仍受上一窗口约束
    EXPECT_FALSE(RateLimiter::allowAt("user:u1", 3, kWindow, kBase + 2 * kWindow - 1));
}

TEST_F(RateLimiterTest, KeysAreIndependent) {
    EXPECT_TRUE(RateLimiter::allowAt("ip:10.0.0.1", 1, kWindow, kBase));
    EXPECT_FALSE(RateLimiter::allowAt("ip:10.0.0.1", 1, kWindow, kBase));
    // 另一个键不受影响
    EXPECT_TRUE(RateLimiter::allowAt("ip:10.0.0.2", 1, kWindow, kBase));
}

TEST_F(RateLimiterTest, NonPositiveLimitMeansUnlimited) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(RateLimiter::allowAt("ip:unlimited", 0, kWindow, kBase));
        EXPECT_TRUE(RateLimiter::allowAt("ip:unlimited-negative", -1, kWindow, kBase));
    }
}

TEST_F(RateLimiterTest, ResetClearsAllCounters) {
    EXPECT_TRUE(RateLimiter::allowAt("ip:1.1.1.1", 1, kWindow, kBase));
    EXPECT_FALSE(RateLimiter::allowAt("ip:1.1.1.1", 1, kWindow, kBase));

    RateLimiter::reset();
    EXPECT_TRUE(RateLimiter::allowAt("ip:1.1.1.1", 1, kWindow, kBase));
}

TEST_F(RateLimiterTest, SweepKeepsActiveWindowsIntact) {
    // 活跃键：窗口起点 kBase（60s 窗口）
    EXPECT_TRUE(RateLimiter::allowAt("user:active", 1, kWindow, kBase));
    // 制造大量短窗口（1s）过期键，推动清扫（kSweepInterval = 1024）
    for (int i = 0; i < 1100; ++i) {
        RateLimiter::allowAt("stale:" + std::to_string(i), 1, 1, kBase);
    }
    // 清扫后活跃键仍记得本窗口内的计数 → 继续拒绝
    EXPECT_FALSE(RateLimiter::allowAt("user:active", 1, kWindow, kBase + 30));
    // 已过窗口的键重新访问按新窗口放行（行为与未清扫时一致）
    EXPECT_TRUE(RateLimiter::allowAt("stale:0", 1, 1, kBase + 30));
}
