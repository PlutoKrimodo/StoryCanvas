#include "GenerationBatchController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../dto/ConversationDto.h"
#include "../services/GenerationBatchService.h"

namespace {

json parseBody(const HttpRequestPtr &req) {
    try {
        const auto &body = req->getBody();
        if (body.empty()) return nullptr;
        return nlohmann::json::parse(body);
    } catch (const std::exception &e) {
        spdlog::warn("解析分镜请求体失败: {}", e.what());
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

json validationErrors(const std::vector<FieldError> &errors) {
    json arr = json::array();
    for (const auto &error : errors) {
        arr.push_back({{"field", error.field}, {"message", error.message}});
    }
    return arr;
}

void respondValidation(const std::function<void(const HttpResponsePtr &)> &callback,
                       const std::vector<FieldError> &errors) {
    callback(ApiResponse::ok(
        ApiResponse::validationError("字段校验失败", validationErrors(errors)),
        static_cast<drogon::HttpStatusCode>(422)));
}

}  // namespace

void GenerationBatchController::createPlan(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }
    auto createReq = CreatePlanRequest::fromJson(body);
    if (!createReq.validate()) {
        respondValidation(callback, createReq.getErrors());
        return;
    }
    respond(callback, GenerationBatchService::createPlan(userId, isGuest, createReq));
}

void GenerationBatchController::updatePlan(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string batchId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");

    auto body = parseBody(req);
    if (body.is_null()) {
        callback(ApiResponse::fail(400, "请求体必须是 JSON"));
        return;
    }
    auto updateReq = UpdatePlanRequest::fromJson(body);
    if (!updateReq.validate()) {
        respondValidation(callback, updateReq.getErrors());
        return;
    }
    respond(callback, GenerationBatchService::updatePlan(userId, batchId, updateReq));
}

void GenerationBatchController::startBatch(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string batchId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");

    auto body = parseBody(req);
    const auto startReq = StartBatchRequest::fromJson(body.is_null() ? json::object() : body);
    if (!startReq.validate()) {
        respondValidation(callback, startReq.getErrors());
        return;
    }
    respond(callback,
            GenerationBatchService::startBatch(userId, isGuest, batchId, startReq));
}

void GenerationBatchController::getBatch(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string batchId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, GenerationBatchService::getBatch(userId, batchId));
}

void GenerationBatchController::cancelBatch(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string batchId) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    respond(callback, GenerationBatchService::cancelBatch(userId, batchId));
}

void GenerationBatchController::regeneratePanel(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string batchId,
    std::string panelIndex) {
    const auto userId = req->getAttributes()->get<std::string>("user_id");
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");

    int index = -1;
    try {
        index = std::stoi(panelIndex);
    } catch (const std::exception &) {
        callback(ApiResponse::fail(422, "分镜序号非法"));
        return;
    }
    if (index < 0 || index > 5) {
        callback(ApiResponse::fail(422, "分镜序号需在 0–5 之间"));
        return;
    }
    respond(callback,
            GenerationBatchService::regeneratePanel(userId, isGuest, batchId, index));
}
