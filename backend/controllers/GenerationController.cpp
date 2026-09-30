#include "GenerationController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/GenerationDto.h"
#include "../services/GenerationService.h"

namespace {

const std::string kPrefix = "/api/v1/generation/";

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

/** 去掉末尾的 `/<suffix>`，返回任务 ID；不匹配则返回空串。 */
std::string taskIdWithSuffix(const std::string &rest, const std::string &suffix) {
    const std::string tail = "/" + suffix;
    if (rest.size() > tail.size() && rest.compare(rest.size() - tail.size(), tail.size(), tail) == 0) {
        return rest.substr(0, rest.size() - tail.size());
    }
    return "";
}

}  // namespace

void GenerationController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    const std::function<void(const HttpResponsePtr &)> fallback = callback;

    try {
        const auto path = req->getPath();
        const auto method = req->getMethod();

        if (path == "/api/v1/generation" && method == Post) {
            createTask(req, std::move(callback));
        } else if (path == "/api/v1/generation/history" && method == Get) {
            listHistory(req, std::move(callback));
        } else if (path.rfind(kPrefix, 0) == 0) {
            const std::string rest = path.substr(kPrefix.size());
            const std::string cancelId = taskIdWithSuffix(rest, "cancel");
            const std::string regenerateId = taskIdWithSuffix(rest, "regenerate");

            if (!cancelId.empty() && method == Post) {
                cancelTask(req, std::move(callback));
            } else if (!regenerateId.empty() && method == Post) {
                regenerate(req, std::move(callback));
            } else if (method == Get) {
                getTask(req, std::move(callback));
            } else {
                fallback(ApiResponse::fail(404, "接口不存在"));
            }
        } else {
            fallback(ApiResponse::fail(404, "接口不存在"));
        }
    } catch (const std::exception &e) {
        spdlog::error("生成接口异常: path={}, what={}", req->getPath(), e.what());
        fallback(ApiResponse::fail(500, "服务内部错误"));
    }
}

void GenerationController::createTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

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

    auto result = GenerationService::createTask(userId, createReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::getTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string taskId = req->getPath().substr(kPrefix.size());

    auto result = GenerationService::getTask(userId, taskId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
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

void GenerationController::cancelTask(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string taskId = taskIdWithSuffix(req->getPath().substr(kPrefix.size()), "cancel");

    auto result = GenerationService::cancelTask(userId, taskId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void GenerationController::regenerate(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string taskId =
        taskIdWithSuffix(req->getPath().substr(kPrefix.size()), "regenerate");

    auto body = parseBody(req);
    auto regenReq = RegenerateRequest::fromJson(body.is_null() ? json::object() : body);

    auto result = GenerationService::regenerate(userId, taskId, regenReq);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::created(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}
