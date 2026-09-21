#include "AuthController.h"
#include "../dto/AuthDto.h"
#include "../dto/ApiResponse.h"
#include "../services/UserService.h"
#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

namespace {
json parseBody(const HttpRequestPtr &req) {
    try {
        auto body = req->getBody();
        if (body.empty()) return nullptr;
        return nlohmann::json::parse(body);
    } catch (const std::exception &e) {
        spdlog::error("解析请求体失败: {}", e.what());
        return nullptr;
    }
}

drogon::HttpResponsePtr sendJson(int code, const json &body) {
    return ApiResponse::ok(body, static_cast<drogon::HttpStatusCode>(code));
}
}  // namespace

void AuthController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    // 分发时会把 callback move 进各处理函数，故先保留一份副本用于异常兜底，
    // 避免在 catch 中调用已被移动的对象
    const std::function<void(const HttpResponsePtr &)> fallback = callback;

    try {
        auto path = req->getPath();

        if (path == "/api/v1/auth/register") {
            registerUser(req, std::move(callback));
        } else if (path == "/api/v1/auth/login") {
            loginUser(req, std::move(callback));
        } else if (path == "/api/v1/auth/refresh") {
            refreshToken(req, std::move(callback));
        } else if (path == "/api/v1/auth/logout") {
            logout(req, std::move(callback));
        } else {
            fallback(ApiResponse::fail(404, "接口不存在"));
        }
    } catch (const std::exception &e) {
        // 兜底：记录异常并返回统一 500，避免 Drogon 直接中断请求
        spdlog::error("认证接口异常: path={}, what={}", req->getPath(), e.what());
        fallback(ApiResponse::fail(500, "服务内部错误"));
    }
}

void AuthController::registerUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto registerReq = RegisterRequest::fromJson(body);

    if (!registerReq.validate()) {
        auto errors = registerReq.getErrors();
        json errorsJson = json::array();
        for (const auto &err : errors) {
            errorsJson.push_back({{"field", err.field}, {"message", err.message}});
        }
        callback(sendJson(422, ApiResponse::validationError("字段校验失败", errorsJson)));
        return;
    }

    spdlog::debug("注册请求: username={}", registerReq.username);
    auto result = UserService::registerUser(registerReq);
    spdlog::debug("注册完成: success={}, code={}", result.success, result.code);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void AuthController::loginUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto loginReq = LoginRequest::fromJson(body);

    if (!loginReq.validate()) {
        auto errors = loginReq.getErrors();
        json errorsJson = json::array();
        for (const auto &err : errors) {
            errorsJson.push_back({{"field", err.field}, {"message", err.message}});
        }
        callback(sendJson(422, ApiResponse::validationError("字段校验失败", errorsJson)));
        return;
    }

    auto result = UserService::loginUser(loginReq);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void AuthController::refreshToken(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto refreshReq = RefreshRequest::fromJson(body);

    if (!refreshReq.validate()) {
        callback(ApiResponse::fail(400, "refresh_token 不能为空"));
        return;
    }

    auto result = UserService::refreshToken(refreshReq.refreshToken);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void AuthController::logout(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto authHeader = req->getHeader("Authorization");
    std::string accessToken;
    if (authHeader.find("Bearer ") == 0) {
        accessToken = authHeader.substr(7);
    }
    
    UserService::logout(accessToken, "");

    callback(ApiResponse::ok(ApiResponse::success("退出成功")));
}
