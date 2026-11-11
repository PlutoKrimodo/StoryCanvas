#include "UserAiCredentialController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../services/UserAiCredentialService.h"

namespace {

json parseBody(const HttpRequestPtr &req) {
    try {
        const auto &body = req->getBody();
        if (body.empty()) return nullptr;
        return nlohmann::json::parse(body);
    } catch (const std::exception &e) {
        // 请求体可能含 Key，禁止把原始内容写进日志
        spdlog::warn("解析 AI 凭据请求体失败: {}", e.what());
        return nullptr;
    }
}

void respond(const std::function<void(const HttpResponsePtr &)> &callback,
             const ServiceResult &result) {
    if (result.success) {
        callback(ApiResponse::ok(
            ApiResponse::success(result.message, result.data),
            static_cast<drogon::HttpStatusCode>(result.code)));
    } else {
        callback(ApiResponse::fail(result.code, result.message, result.errorCode));
    }
}

}  // namespace

void UserAiCredentialController::overview(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, UserAiCredentialService::overview(userId));
}

void UserAiCredentialController::providers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    respond(callback, UserAiCredentialService::providers());
}

void UserAiCredentialController::save(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string capability) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }
    respond(callback, UserAiCredentialService::save(userId, capability, body));
}

void UserAiCredentialController::remove(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string capability) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, UserAiCredentialService::remove(userId, capability));
}

void UserAiCredentialController::test(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }
    respond(callback, UserAiCredentialService::test(userId, body));
}
