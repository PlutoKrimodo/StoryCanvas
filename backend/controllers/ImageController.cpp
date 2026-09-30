#include "ImageController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../services/ImageService.h"

namespace {

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

drogon::HttpResponsePtr sendBinary(const ImageBinary &binary, bool asAttachment) {
    if (!binary.ok) {
        return ApiResponse::fail(binary.code, binary.message);
    }
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeString(binary.mimeType);
    resp->setBody(binary.content);
    if (asAttachment) {
        resp->addHeader("Content-Disposition",
                        "attachment; filename=\"" + binary.filename + "\"");
    }
    return resp;
}

}  // namespace

// ==================== 处理函数 ====================
// 路由由 ImageController.h 的 ADD_METHOD_TO 直接分发；
// 路径参数 imageId 由 Drogon 依据 {1} 解析后传入。

void ImageController::listImages(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    const int page = toInt(req->getParameter("page"), 1);
    const int limit = toInt(req->getParameter("limit"), 20);
    const std::string bookId = req->getParameter("book_id");

    auto result = ImageService::listImages(userId, page, limit, bookId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void ImageController::getImage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string imageId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = ImageService::getImage(userId, imageId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void ImageController::deleteImage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string imageId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    auto result = ImageService::deleteImage(userId, imageId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void ImageController::sendRaw(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string imageId) {

    callback(sendBinary(ImageService::loadImageFile(imageId), false));
}

void ImageController::download(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string imageId) {

    auto userId = req->getAttributes()->get<std::string>("user_id");

    // 先做归属校验，再读取文件；避免非本人直接下载
    auto owned = ImageService::getImage(userId, imageId);
    if (!owned.success) {
        callback(sendJson(owned.code, ApiResponse::error(owned.code, owned.message)));
        return;
    }
    callback(sendBinary(ImageService::loadImageFile(imageId), true));
}
