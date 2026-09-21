#include "JwtFilter.h"
#include "../dto/ApiResponse.h"
#include "../utils/JwtUtils.h"
#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

void JwtFilter::doFilter(const HttpRequestPtr &req, FilterCallback &&fcb, FilterChainCallback &&fccb) {
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        fcb(ApiResponse::fail(401, "未提供有效的认证令牌"));
        return;
    }

    std::string token = authHeader.substr(7);
    if (token.empty()) {
        fcb(ApiResponse::fail(401, "Token 为空"));
        return;
    }

    auto payloadOpt = JwtUtils::verifyToken(token, "access");
    if (!payloadOpt) {
        fcb(ApiResponse::fail(401, "Token 无效或已过期"));
        return;
    }

    if (JwtUtils::isRevoked(payloadOpt->jti)) {
        fcb(ApiResponse::fail(401, "Token 已被撤销"));
        return;
    }

    auto attributes = req->getAttributes();
    attributes->insert("user_id", payloadOpt->userId);
    attributes->insert("username", payloadOpt->username);

    fccb();
}
