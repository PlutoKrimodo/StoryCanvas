#pragma once

#include <ctime>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

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
 * - 支持两种撤销：按 jti 精确撤销单个 Token，以及按 userId 整批撤销
 *   （修改密码后强制该用户全部会话重新登录）
 * - 撤销记录为内存黑名单，后续可平滑迁移到 Redis
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

    /**
     * 撤销某用户在此刻之前签发的全部 Token（Access + Refresh）
     *
     * 用途：修改密码后强制该用户所有会话重新登录（见 docs/16 第 6.6.1 节）。
     *
     * 注意：JWT 的 iat 只有秒级精度，因此撤销粒度为 1 秒——若用户在同一秒内
     * 完成「改密 + 重新登录」，新 Token 会被一并拒绝，需再次登录（可接受）。
     * 撤销记录保存在进程内存，重启后失效；MVP 阶段可接受，后续可迁移到 Redis。
     */
    static void revokeAllForUser(const std::string &userId);

    /** 判断某用户在 issuedAt 时刻签发的 Token 是否已因整用户撤销而失效 */
    static bool isUserTokenRevoked(const std::string &userId, time_t issuedAt);

    /** 清空全部撤销记录（仅供测试使用） */
    static void resetRevocations();

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
    /** userId -> 撤销生效时间戳，早于该时刻签发的 Token 一律失效 */
    static std::unordered_map<std::string, time_t> userRevokedBefore_;
};
