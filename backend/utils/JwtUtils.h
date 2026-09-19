#pragma once

#include <ctime>
#include <mutex>
#include <optional>
#include <set>
#include <string>

/** 解析后的 Token 载荷 */
struct TokenPayload {
    std::string userId;
    std::string username;
    std::string type;  // "access" | "refresh"
    std::string jti;   // Token 唯一标识，用于撤销
    time_t issuedAt = 0;
    time_t expiresAt = 0;
};

/** Access / Refresh 双 Token */
struct TokenPair {
    std::string accessToken;
    std::string refreshToken;
};

/** 刷新结果 */
struct RefreshResult {
    std::string accessToken;
    std::string refreshToken;
};

/**
 * JWT 工具（HS256）
 *
 * - 自行实现 HS256 签发与校验，底层使用 OpenSSL 的 HMAC-SHA256，
 *   避免额外引入 jwt-cpp 依赖
 * - 双 Token：Access（短寿命）+ Refresh（长寿命）
 * - 支持按 jti 撤销（内存黑名单，后续可平滑迁移到 Redis）
 */
class JwtUtils {
public:
    static void init(const std::string &secret, int accessTokenExpiry, int refreshTokenExpiry);

    static TokenPair generateTokenPair(const std::string &userId, const std::string &username);

    /** 校验 Token；expectedType 为空表示不校验类型（如刷新场景） */
    static std::optional<TokenPayload> verifyToken(const std::string &token,
                                                   const std::string &expectedType = "access");

    /** 使用 Refresh Token 换发新的双 Token */
    static std::optional<RefreshResult> refresh(const std::string &refreshToken);

    /** 将 Token 加入撤销黑名单 */
    static void revokeToken(const std::string &token);

    /** 判断 jti 是否已被撤销 */
    static bool isRevoked(const std::string &jti);

private:
    static std::string encodeSegment(const std::string &json);
    static std::string sign(const std::string &signingInput);
    static std::string buildToken(const std::string &userId,
                                  const std::string &username,
                                  const std::string &type,
                                  const std::string &jti,
                                  int expirySeconds);

    static std::string secret_;
    static int accessTokenExpiry_;
    static int refreshTokenExpiry_;

    static std::mutex mutex_;
    static std::set<std::string> revokedJtis_;
};
