#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

/** 页容器接口：保存到绘本 / 读取页列表（docs/04 §5）。 */
class PageController : public drogon::HttpSimpleController<PageController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/books/{book_id}/pages", Post, "JwtFilter");
    PATH_ADD("/api/v1/books/{book_id}/pages", Get, "JwtFilter");
    PATH_LIST_END

private:
    void savePage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void listPages(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
