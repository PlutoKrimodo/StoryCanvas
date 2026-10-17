#include "ConversationController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/ConversationDto.h"
#include "../services/ConversationService.h"

namespace {

json parseBody(const HttpRequestPtr &req) {
    try {
        const auto &body = req->getBody();
        if (body.empty()) return nullptr;
        return nlohmann::json::parse(body);
    } catch (const std::exception &e) {
        spdlog::warn("解析会话请求体失败: {}", e.what());
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
        callback(ApiResponse::fail(result.code, result.message));
    }
}

int toInt(const std::string &value, int fallback) {
    if (value.empty()) return fallback;
    try {
        return std::stoi(value);
    } catch (const std::exception &) {
        return fallback;
    }
}

}  // namespace

void ConversationController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto createReq = CreateConversationRequest::fromJson(body);
    if (!createReq.validate()) {
        callback(ApiResponse::fail(422, "会话字段校验失败"));
        return;
    }
    respond(callback, ConversationService::create(userId, createReq));
}

void ConversationController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    const int page = toInt(req->getParameter("page"), 1);
    const int limit = toInt(req->getParameter("limit"), 20);
    respond(callback, ConversationService::list(userId, page, limit));
}

void ConversationController::get(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string conversationId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, ConversationService::get(userId, conversationId));
}

void ConversationController::remove(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string conversationId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, ConversationService::remove(userId, conversationId));
}
