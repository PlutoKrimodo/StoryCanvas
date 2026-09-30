#include "GenerationController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/GenerationDto.h"
#include "../services/GenerationService.h"

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

int toInt(const std::string &value, int defaultValue) {
    if (value.empty()) return defaultValue;
    try {
        return std::stoi(value);
    } catch (const std::exception &) {
        return defaultValue;
    }
}

json validationErrors(const std::vector<FieldError> &errors) {
    json arr = json::array();
    for (const auto &err : errors) {
        arr.push_back({{"field", err.field}, {"message", err.message}});
    }
    return arr;
}

}  // namespace

// ==================== 处理函数 ====================
// 路由由 GenerationController.h 的 ADD_METHOD_TO 直接分发到下列方法；
// 路径参数 taskId 由 Drogon 依据 {1} 解析后传入，无需再从 req->getPath() 手工截取。
// 处理器抛出的异常由 Drogon 的 HttpBinder 统一兜底（返回 500）。

void GenerationController::createTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto createReq = CreateGenerationRequest::fromJson(body);
    if (!createReq.validate()) {
        callback(sendJson(422, ApiResponse::validationError(
                                   "字段校验失败", validationErrors(createReq.getErrors()))));
        return;
    }

    auto result = GenerationService::createTask(userId, isGuest, createReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::listHistory(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    const int page = toInt(req->getParameter("page"), 1);
    const int limit = toInt(req->getParameter("limit"), 20);
    const std::string bookId = req->getParameter("book_id");

    auto result = GenerationService::listHistory(userId, page, limit, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::getTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string taskId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = GenerationService::getTask(userId, taskId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::cancelTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string taskId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = GenerationService::cancelTask(userId, taskId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::regenerate(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string taskId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");

    auto body = parseBody(req);
    auto regenReq = RegenerateRequest::fromJson(body.is_null() ? json::object() : body);

    auto result = GenerationService::regenerate(userId, isGuest, taskId, regenReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
