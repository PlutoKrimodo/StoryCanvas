#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

/**
 * 图片接口（docs/04 §9）。
 *
 * 其中 `/raw` 不挂 JwtFilter：它要供前端 `<img src>` 直接引用，
 * 浏览器无法为图片请求附加 Authorization 头。图片 ID 为 UUID，
 * 视为不可枚举的公开资源；下载与删除仍走 JWT 鉴权。
 */
class ImageController : public drogon::HttpSimpleController<ImageController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/images", Get, "JwtFilter");
    PATH_ADD("/api/v1/images/{image_id}", Get, "JwtFilter");
    PATH_ADD("/api/v1/images/{image_id}", Delete, "JwtFilter");
    PATH_ADD("/api/v1/images/{image_id}/download", Get, "JwtFilter");
    PATH_ADD("/api/v1/images/{image_id}/raw", Get);
    PATH_LIST_END

private:
    void getImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void listImages(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void sendRaw(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void download(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
