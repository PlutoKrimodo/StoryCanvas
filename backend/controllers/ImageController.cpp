#include "ImageController.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

#include "../dto/ApiResponse.h"
#include "../services/ImageService.h"

namespace {

const std::string kPrefix = "/api/v1/images";

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

/** 从 `/api/v1/images/{id}/<suffix>` 中取出 {id}；不匹配返回空串。 */
std::string idWithSuffix(const std::string &path, const std::string &suffix) {
    const std::string tail = "/" + suffix;
    if (path.size() > tail.size() &&
        path.compare(path.size() - tail.size(), tail.size(), tail) == 0) {
        return path.substr(kPrefix.size() + 1, path.size() - kPrefix.size() - 1 - tail.size());
    }
    return "";
}

/** 裸路径 `/api/v1/images/{id}` 中的 {id}；含子路径时返回空串。 */
std::string plainId(const std::string &path) {
    if (path.rfind(kPrefix + "/", 0) != 0) {
        return "";
    }
    const std::string rest = path.substr(kPrefix.size() + 1);
    if (rest.empty() || rest.find('/') != std::string::npos) {
        return "";
    }
    return rest;
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

void ImageController::asyncHandleHttpRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    const std::function<void(const HttpResponsePtr &)> fallback = callback;

    try {
        const auto path = req->getPath();
        const auto method = req->getMethod();

        if (path == "/api/v1/images" && method == Get) {
            listImages(req, std::move(callback));
            return;
        }

        if (!idWithSuffix(path, "raw").empty() && method == Get) {
            sendRaw(req, std::move(callback));
            return;
        }
        if (!idWithSuffix(path, "download").empty() && method == Get) {
            download(req, std::move(callback));
            return;
        }

        const std::string id = plainId(path);
        if (!id.empty() && method == Get) {
            getImage(req, std::move(callback));
            return;
        }
        if (!id.empty() && method == Delete) {
            deleteImage(req, std::move(callback));
            return;
        }

        fallback(ApiResponse::fail(404, "接口不存在"));
    } catch (const std::exception &e) {
        spdlog::error("图片接口异常: path={}, what={}", req->getPath(), e.what());
        fallback(ApiResponse::fail(500, "服务内部错误"));
    }
}

void ImageController::getImage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string imageId = plainId(req->getPath());

    auto result = ImageService::getImage(userId, imageId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message, result.data)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

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

void ImageController::deleteImage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string imageId = plainId(req->getPath());

    auto result = ImageService::deleteImage(userId, imageId);
    if (result.success) {
        callback(sendJson(result.code, ApiResponse::success(result.message)));
    } else {
        callback(sendJson(result.code, ApiResponse::error(result.code, result.message)));
    }
}

void ImageController::sendRaw(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    const std::string imageId = idWithSuffix(req->getPath(), "raw");
    callback(sendBinary(ImageService::loadImageFile(imageId), false));
}

void ImageController::download(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

    auto userId = req->getAttributes()->get<std::string>("user_id");
    const std::string imageId = idWithSuffix(req->getPath(), "download");

    // 先做归属校验，再读取文件；避免非本人直接下载
    auto owned = ImageService::getImage(userId, imageId);
    if (!owned.success) {
        callback(sendJson(owned.code, ApiResponse::error(owned.code, owned.message)));
        return;
    }
    callback(sendBinary(ImageService::loadImageFile(imageId), true));
}
