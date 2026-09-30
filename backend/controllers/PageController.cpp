#include "PageController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/PageDto.h"
#include "../services/PageService.h"

namespace {

// 单本绘本路径前缀与页后缀：/api/v1/books/{book_id}/pages
const std::string kBookPrefix = "/api/v1/books/";
const std::string kPagesSuffix = "/pages";

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

bool isPagesPath(const std::string &path) {
    if (path.rfind(kBookPrefix, 0) != 0) {
        return false;
    }
    if (path.size() <= kBookPrefix.size() + kPagesSuffix.size()) {
        return false;
    }
    return path.compare(path.size() - kPagesSuffix.size(), kPagesSuffix.size(), kPagesSuffix) == 0;
}

std::string extractBookId(const std::string &path) {
    if (!isPagesPath(path)) {
        return "";
    }
    const size_t end = path.size() - kPagesSuffix.size();
    return path.substr(kBookPrefix.size(), end - kBookPrefix.size());
}

json validationErrors(const std::vector<FieldError> &errors) {
    json arr = json::array();
    for (const auto &err : errors) {
        arr.push_back({{"field", err.field}, {"message", err.message}});
    }
    return arr;
}

}  // namespace

void PageController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    const std::function<void(const HttpResponsePtr &)> fallback = callback;

    try {
        const auto path = req->getPath();
        const auto method = req->getMethod();

        if (isPagesPath(path)) {
            if (method == Post) {
                savePage(req, std::move(callback));
            } else if (method == Get) {
                listPages(req, std::move(callback));
            } else {
                fallback(ApiResponse::fail(404, "接口不存在"));
            }
        } else {
            fallback(ApiResponse::fail(404, "接口不存在"));
        }
    } catch (const std::exception &e) {
        spdlog::error("页容器接口异常: path={}, what={}", req->getPath(), e.what());
        fallback(ApiResponse::fail(500, "服务内部错误"));
    }
}

void PageController::savePage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string bookId = extractBookId(req->getPath());

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
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string bookId = extractBookId(req->getPath());

    auto result = PageService::listPages(userId, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
