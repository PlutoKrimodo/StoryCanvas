#include "PageController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/PageDto.h"
#include "../services/PageService.h"

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

json validationErrors(const std::vector<FieldError> &errors) {
    json arr = json::array();
    for (const auto &err : errors) {
        arr.push_back({{"field", err.field}, {"message", err.message}});
    }
    return arr;
}

}  // namespace

// ==================== 处理函数 ====================
// 路由由 PageController.h 的 ADD_METHOD_TO 直接分发；
// 路径参数 bookId 由 Drogon 依据 {1} 解析后传入。

void PageController::savePage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }

    auto saveReq = SavePageRequest::fromJson(body);
    if (!saveReq.validate()) {
        callback(sendJson(422, ApiResponse::validationError(
                                   "字段校验失败", validationErrors(saveReq.getErrors()))));
        return;
    }

    auto result = PageService::savePage(userId, bookId, saveReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void PageController::listPages(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = PageService::listPages(userId, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
