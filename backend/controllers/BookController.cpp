#include "BookController.h"

#include "../dto/ApiResponse.h"
#include "../dto/BookDto.h"
#include "../services/BookService.h"
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
// 路由由 BookController.h 的 ADD_METHOD_TO 直接分发；
// 路径参数 bookId 由 Drogon 依据 {1} 解析后传入。

void BookController::createBook(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto createReq = CreateBookRequest::fromJson(body);
    if (!createReq.validate()) {
        callback(sendJson(422, ApiResponse::validationError(
                                   "字段校验失败", validationErrors(createReq.getErrors()))));
        return;
    }

    auto result = BookService::createBook(userId, createReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void BookController::listMyBooks(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    const int page = toInt(req->getParameter("page"), 1);
    const int limit = toInt(req->getParameter("limit"), 20);
    std::string status = req->getParameter("status");
    // status 仅接受合法枚举，非法或缺失一律视为不筛选
    if (status != "draft" && status != "published" && status != "archived") {
        status.clear();
    }

    auto result = BookService::listBooks(userId, page, limit, status);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void BookController::getBook(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = BookService::getBook(userId, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void BookController::updateBook(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto updateReq = UpdateBookRequest::fromJson(body);
    if (!updateReq.validate()) {
        callback(sendJson(422, ApiResponse::validationError(
                                   "字段校验失败", validationErrors(updateReq.getErrors()))));
        return;
    }

    auto result = BookService::updateBook(userId, bookId, updateReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void BookController::deleteBook(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = BookService::deleteBook(userId, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
