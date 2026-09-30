#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * 图片接口（docs/04 §9）。
 *
 * 其中 `/raw` 不挂 JwtFilter：它要供前端 `<img src>` 直接引用，
 * 浏览器无法为图片请求附加 Authorization 头。图片 ID 为 UUID，
 * 视为不可枚举的公开资源；下载与删除仍走 JWT 鉴权。
 *
 * 路由注册必须用 HttpController + ADD_METHOD_TO（原因见 GenerationController.h 注释）：
 * HttpSimpleController 的 PATH_ADD 只做精确路径匹配，{image_id} 不会命中真实请求。
 * {1} 必须对应 handler 的一个参数，否则启动即 exit(1)。
 */
class ImageController : public drogon::HttpController<ImageController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ImageController::listImages, "/api/v1/images", Get, "JwtFilter");
    ADD_METHOD_TO(ImageController::getImage, "/api/v1/images/{1}", Get, "JwtFilter");
    ADD_METHOD_TO(ImageController::deleteImage, "/api/v1/images/{1}", Delete, "JwtFilter");
    ADD_METHOD_TO(ImageController::download, "/api/v1/images/{1}/download", Get, "JwtFilter");
    ADD_METHOD_TO(ImageController::sendRaw, "/api/v1/images/{1}/raw", Get);
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void listImages(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    void getImage(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string imageId);
    void deleteImage(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     std::string imageId);
    void download(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string imageId);
    void sendRaw(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 std::string imageId);
};
