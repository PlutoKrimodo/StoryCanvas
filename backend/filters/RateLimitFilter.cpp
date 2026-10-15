#include "RateLimitFilter.h"

#include <drogon/HttpAppFramework.h>
#include <spdlog/spdlog.h>

#include <cctype>
#include <cstdlib>
#include <string>

#include "../dto/ApiResponse.h"
#include "../utils/ConfigManager.h"
#include "../utils/JwtUtils.h"
#include "../utils/RateLimiter.h"

namespace {

constexpr int kWindowSeconds = 60;

/** 172.16.0.0/12 才是内网段；仅看 "172." 前缀会把 172.0–15 / 172.32–255 公网段误判为内网 */
bool isPrivate172(const std::string &peer) {
    if (peer.rfind("172.", 0) != 0) {
        return false;
    }
    const int secondOctet = std::atoi(peer.c_str() + 4);
    return secondOctet >= 16 && secondOctet <= 31;
}

/**
 * 客户端 IP 判定。
 *
 * 直连时取对端地址；当对端是回环 / 内网地址（本机 Vite 代理、nginx / Docker
 * 网关）时，优先采用反向代理透传的 X-Real-IP（nginx.conf 已配置）。
 * 局限：来自内网的对端可以伪造 X-Real-IP 绕过 IP 限频，属已知取舍——
 * 成本封顶仍由全站每日额度兜底（docs/18 Q3 结论）。
 */
std::string clientIp(const drogon::HttpRequestPtr &req) {
    std::string peer = req->getPeerAddr().toIp();

    // IPv4-mapped IPv6（::ffff:127.0.0.1）归一为 IPv4
    if (peer.rfind("::ffff:", 0) == 0) {
        peer = peer.substr(7);
    }

    const bool localPeer = peer == "::1" || peer == "localhost" ||
                           peer.rfind("127.", 0) == 0 || peer.rfind("10.", 0) == 0 ||
                           peer.rfind("192.168.", 0) == 0 || isPrivate172(peer);
    if (!localPeer) {
        return peer;
    }

    const std::string realIp = req->getHeader("X-Real-IP");
    return realIp.empty() ? peer : realIp;
}

/** 从 Authorization 头解析有效身份；无效 / 缺失按匿名处理 */
bool resolveUserId(const drogon::HttpRequestPtr &req, std::string &userId) {
    const std::string auth = req->getHeader("Authorization");
    if (auth.rfind("Bearer ", 0) != 0) {
        return false;
    }
    const auto payload = JwtUtils::verifyToken(auth.substr(7), "access");
    if (!payload.has_value() || payload->userId.empty()) {
        return false;
    }
    userId = payload->userId;
    return true;
}

drogon::HttpResponsePtr tooManyRequests(const std::string &rule,
                                        const std::string &key,
                                        const std::string &message) {
    spdlog::warn("限流拦截 | rule={} | key={}", rule, key);
    auto resp = ApiResponse::fail(429, message);
    resp->addHeader("Retry-After", std::to_string(kWindowSeconds));
    return resp;
}

bool isGenerationCreate(const drogon::HttpRequestPtr &req) {
    return req->method() == drogon::Post && req->path() == "/api/v1/generation";
}

/** 重新生成同样触发一次真实计费调用，一并限频 */
bool isGenerationRegenerate(const drogon::HttpRequestPtr &req) {
    const std::string &path = req->path();
    constexpr char kPrefix[] = "/api/v1/generation/";
    constexpr char kSuffix[] = "/regenerate";
    constexpr std::size_t kPrefixLen = sizeof(kPrefix) - 1;
    constexpr std::size_t kSuffixLen = sizeof(kSuffix) - 1;
    return req->method() == drogon::Post && path.size() > kPrefixLen + kSuffixLen &&
           path.rfind(kPrefix, 0) == 0 &&
           path.compare(path.size() - kSuffixLen, kSuffixLen, kSuffix) == 0;
}

}  // namespace

void RateLimitFilter::registerGlobalAdvice() {
    if (!ConfigManager::rateLimitEnabled) {
        spdlog::warn("全局限流已关闭（RATE_LIMIT_ENABLED=false）：仅在开发 / 压测环境使用");
        return;
    }

    if (ConfigManager::rateLimitAnonPerMin <= 0 ||
        ConfigManager::rateLimitAuthPerMin <= 0 ||
        ConfigManager::rateLimitGenerationPerMin <= 0 ||
        ConfigManager::rateLimitGuestTokenPerMin <= 0) {
        spdlog::warn("限流额度存在 <=0 的档位：该档位将不限制");
    }

    drogon::app().registerSyncAdvice(
        [](const drogon::HttpRequestPtr &req) { return check(req); });

    spdlog::info("全局限流已启用 | 匿名 {}/分(IP) | 认证 {}/分(用户) | 生成 {}/分 | 游客会话 {}/分(IP)",
                 ConfigManager::rateLimitAnonPerMin,
                 ConfigManager::rateLimitAuthPerMin,
                 ConfigManager::rateLimitGenerationPerMin,
                 ConfigManager::rateLimitGuestTokenPerMin);
}

drogon::HttpResponsePtr RateLimitFilter::check(const drogon::HttpRequestPtr &req) {
    // 预检请求与健康检查不计入额度
    if (req->method() == drogon::Options) {
        return nullptr;
    }
    const std::string &path = req->path();
    if (path == "/health" || path == "/api/v1/health") {
        return nullptr;
    }
    // 生成图 raw：由 <img> 直连加载（无 Authorization 头），豁免匿名额度
    constexpr char kRawSuffix[] = "/raw";
    constexpr std::size_t kRawLen = sizeof(kRawSuffix) - 1;
    if (req->method() == drogon::Get && path.size() > kRawLen &&
        path.compare(path.size() - kRawLen, kRawLen, kRawSuffix) == 0) {
        return nullptr;
    }

    // 身份：有效 Access Token 按用户限频，否则按 IP（匿名）
    std::string identityKey;
    int identityLimit = ConfigManager::rateLimitAuthPerMin;
    std::string userId;
    if (resolveUserId(req, userId)) {
        identityKey = "user:" + userId;
    } else {
        identityKey = "ip:" + clientIp(req);
        identityLimit = ConfigManager::rateLimitAnonPerMin;
    }

    if (!RateLimiter::allow("general:" + identityKey, identityLimit, kWindowSeconds)) {
        return tooManyRequests("general", identityKey, "请求过于频繁，请稍后再试");
    }

    // 游客会话签发：按 IP 单独限频，防止无限换取游客身份（Q3①）
    if (req->method() == drogon::Post && path == "/api/v1/auth/guest") {
        const std::string ipKey = "ip:" + clientIp(req);
        if (!RateLimiter::allow("guest-token:" + ipKey,
                                ConfigManager::rateLimitGuestTokenPerMin,
                                kWindowSeconds)) {
            return tooManyRequests("guest-token", ipKey,
                                   "游客会话创建过于频繁，请稍后再试");
        }
    }

    // AI 生成：任务创建与重新生成都会产生真实计费调用（Q5）
    if (isGenerationCreate(req) || isGenerationRegenerate(req)) {
        if (!RateLimiter::allow("generation:" + identityKey,
                                ConfigManager::rateLimitGenerationPerMin,
                                kWindowSeconds)) {
            return tooManyRequests(
                "generation", identityKey,
                "生成请求过于频繁（每分钟最多 " +
                    std::to_string(ConfigManager::rateLimitGenerationPerMin) +
                    " 次），请稍后再试");
        }
    }

    return nullptr;
}
