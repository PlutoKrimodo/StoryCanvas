#include "JwtUtils.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <stdexcept>
#include <vector>

#include "Base64.h"
#include "EncryptionUtils.h"

using json = nlohmann::json;

std::string JwtUtils::secret_;
int JwtUtils::accessTokenExpiry_ = 3600;
int JwtUtils::refreshTokenExpiry_ = 604800;
int JwtUtils::guestTokenExpiry_ = 604800;

std::mutex JwtUtils::mutex_;
std::set<std::string> JwtUtils::revokedJtis_;
std::unordered_map<std::string, double> JwtUtils::userRevokedBefore_;

namespace {

constexpr char kHeaderJson[] = R"({"alg":"HS256","typ":"JWT"})";

/** 当前时间（浮点秒，毫秒级以上精度；RFC 7519 NumericDate 允许非整数） */
double nowSeconds() {
    return std::chrono::duration<double>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::vector<std::string> splitToken(const std::string &token) {
    std::vector<std::string> segments;
    size_t start = 0;
    while (true) {
        const size_t pos = token.find('.', start);
        if (pos == std::string::npos) {
            segments.push_back(token.substr(start));
            break;
        }
        segments.push_back(token.substr(start, pos - start));
        start = pos + 1;
    }
    return segments;
}

}  // namespace

void JwtUtils::init(const std::string &secret,
                    int accessTokenExpiry,
                    int refreshTokenExpiry,
                    int guestTokenExpiry) {
    if (secret.empty()) {
        throw std::runtime_error("JWT 密钥为空，请通过 JWT_SECRET 环境变量注入");
    }
    secret_ = secret;
    accessTokenExpiry_ = accessTokenExpiry;
    refreshTokenExpiry_ = refreshTokenExpiry;
    guestTokenExpiry_ = guestTokenExpiry;
}

std::string JwtUtils::encodeSegment(const std::string &text) {
    return Base64::encodeUrl(text);
}

std::string JwtUtils::sign(const std::string &signingInput) {
    return Base64::encodeUrl(EncryptionUtils::hmacSha256(secret_, signingInput));
}

std::string JwtUtils::buildToken(const std::string &userId,
                                 const std::string &username,
                                 const std::string &type,
                                 const std::string &jti,
                                 int expirySeconds,
                                 bool isGuest) {
    spdlog::debug("JWT buildToken: type={}, guest={}", type, isGuest);
    const double now = nowSeconds();

    json payload = {
        {"sub", userId},
        {"username", username},
        {"type", type},
        {"jti", jti},
        {"iat", now},
        {"exp", now + expirySeconds},
    };
    if (isGuest) {
        payload["guest"] = true;
    }

    const std::string headerSegment = encodeSegment(kHeaderJson);
    const std::string payloadSegment = encodeSegment(payload.dump());
    const std::string signingInput = headerSegment + "." + payloadSegment;
    const std::string sig = sign(signingInput);

    return signingInput + "." + sig;
}

TokenPair JwtUtils::generateTokenPair(const std::string &userId, const std::string &username) {
    if (secret_.empty()) {
        throw std::runtime_error("JwtUtils 未初始化");
    }

    // 无状态模式下 iat 相同会导致同秒内生成的 Refresh Token 完全一致，
    // 这里依赖 jti 随机值保证唯一性
    TokenPair pair;
    pair.accessToken = buildToken(userId, username, "access",
                                  EncryptionUtils::randomHex(16), accessTokenExpiry_);
    pair.refreshToken = buildToken(userId, username, "refresh",
                                   EncryptionUtils::randomHex(16), refreshTokenExpiry_);
    return pair;
}

std::string JwtUtils::generateGuestToken(const std::string &userId, const std::string &username) {
    if (secret_.empty()) {
        throw std::runtime_error("JwtUtils 未初始化");
    }
    // 游客 Token 复用 "access" 类型（便于既有 JwtFilter 校验），但携带 guest 声明
    return buildToken(userId, username, "access", EncryptionUtils::randomHex(16),
                      guestTokenExpiry_, /*isGuest=*/true);
}

std::optional<TokenPayload> JwtUtils::verifyToken(const std::string &token,
                                                  const std::string &expectedType) {
    if (token.empty() || secret_.empty()) {
        return std::nullopt;
    }

    const std::vector<std::string> segments = splitToken(token);
    if (segments.size() != 3) {
        return std::nullopt;
    }

    const std::string signingInput = segments[0] + "." + segments[1];
    const std::string expectedSignature = sign(signingInput);
    if (!EncryptionUtils::constantTimeEquals(expectedSignature, segments[2])) {
        return std::nullopt;
    }

    json payload;
    try {
        payload = json::parse(Base64::decodeUrl(segments[1]));
    } catch (const std::exception &) {
        return std::nullopt;
    }

    TokenPayload result;
    try {
        result.userId = payload.at("sub").get<std::string>();
        result.username = payload.value("username", "");
        result.type = payload.value("type", "access");
        result.jti = payload.value("jti", "");
        result.isGuest = payload.value("guest", false);
        result.issuedAt = payload.at("iat").get<double>();
        result.expiresAt = payload.at("exp").get<double>();
    } catch (const std::exception &) {
        return std::nullopt;
    }

    if (nowSeconds() >= result.expiresAt) {
        return std::nullopt;
    }

    if (!expectedType.empty() && result.type != expectedType) {
        return std::nullopt;
    }

    if (isRevoked(result.jti)) {
        return std::nullopt;
    }

    // 整用户撤销（如修改密码后）：早于撤销时刻签发的 Token 一律失效
    if (isUserTokenRevoked(result.userId, result.issuedAt)) {
        return std::nullopt;
    }

    return result;
}

std::optional<RefreshResult> JwtUtils::refresh(const std::string &refreshToken) {
    const auto payload = verifyToken(refreshToken, "refresh");
    if (!payload.has_value()) {
        return std::nullopt;
    }

    const TokenPair pair = generateTokenPair(payload->userId, payload->username);

    // 轮换 Refresh Token：旧 Token 立即作废
    revokeToken(refreshToken);

    RefreshResult result;
    result.accessToken = pair.accessToken;
    result.refreshToken = pair.refreshToken;
    return result;
}

void JwtUtils::revokeToken(const std::string &token) {
    const std::vector<std::string> segments = splitToken(token);
    if (segments.size() != 3) {
        return;
    }

    std::string jti;
    try {
        const json payload = json::parse(Base64::decodeUrl(segments[1]));
        jti = payload.value("jti", "");
    } catch (const std::exception &) {
        return;
    }

    if (jti.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    revokedJtis_.insert(jti);
}

bool JwtUtils::isRevoked(const std::string &jti) {
    if (jti.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return revokedJtis_.count(jti) > 0;
}

void JwtUtils::revokeAllForUser(const std::string &userId) {
    if (userId.empty()) {
        return;
    }
    const double revokedAt = nowSeconds();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        userRevokedBefore_[userId] = revokedAt;
    }
    // 日志不记录 userId 与 Token，避免身份信息与凭据落盘
    spdlog::info("JWT: 已撤销某用户全部历史 Token");
}

bool JwtUtils::isUserTokenRevoked(const std::string &userId, double issuedAt) {
    if (userId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = userRevokedBefore_.find(userId);
    if (it == userRevokedBefore_.end()) {
        return false;
    }
    // 撤销时刻与 iat 同为浮点秒（毫秒级以上精度）：撤销瞬时及之前的 Token 全部失效，
    // 之后签发的新 Token（如改密后立即重登）不再被误伤
    return issuedAt <= it->second;
}

void JwtUtils::resetRevocations() {
    std::lock_guard<std::mutex> lock(mutex_);
    revokedJtis_.clear();
    userRevokedBefore_.clear();
}
