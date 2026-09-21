#include "UserController.h"
#include "../dto/UserDto.h"
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

void UserController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto path = req->getPath();
    auto method = req->getMethod();

    if (path == "/api/v1/users/me" && method == Get) {
        getMe(req, std::move(callback));
    } else if (path == "/api/v1/users/me" && method == Put) {
        updateMe(req, std::move(callback));
    } else if (path == "/api/v1/users/me/password" && method == Put) {
        changePassword(req, std::move(callback));
    } else {
        callback(ApiResponse::fail(404, "接口不存在"));
    }
}

void UserController::getMe(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = UserService::getUserById(userId);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void UserController::updateMe(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto updateReq = UpdateUserRequest::fromJson(body);

    if (!updateReq.validate()) {
        auto errors = updateReq.getErrors();
        json errorsJson = json::array();
        for (const auto &err : errors) {
            errorsJson.push_back({{"field", err.field}, {"message", err.message}});
        }
        callback(sendJson(422, ApiResponse::validationError("字段校验失败", errorsJson)));
        return;
    }

    auto result = UserService::updateUser(userId, updateReq);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void UserController::changePassword(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto changeReq = ChangePasswordRequest::fromJson(body);

    if (!changeReq.validate()) {
        auto errors = changeReq.getErrors();
        json errorsJson = json::array();
        for (const auto &err : errors) {
            errorsJson.push_back({{"field", err.field}, {"message", err.message}});
        }
        callback(sendJson(422, ApiResponse::validationError("字段校验失败", errorsJson)));
        return;
    }

    auto result = UserService::changePassword(userId, changeReq);

    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
