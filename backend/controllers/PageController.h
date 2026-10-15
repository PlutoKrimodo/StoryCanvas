#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * 页容器接口：保存到绘本 / 读取页列表（docs/04 §5）。
 *
 * 路由注册必须用 HttpController + ADD_METHOD_TO（原因见 GenerationController.h 注释）：
 * HttpSimpleController 的 PATH_ADD 只做精确路径匹配，{book_id} 不会命中真实请求。
 * {1} 必须对应 handler 的一个参数，否则启动即 exit(1)。
 *
 * 保存页（「保存到绘本」）额外挂 MemberFilter（Q3②）：游客不能写绘本。
 */
class PageController : public drogon::HttpController<PageController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PageController::savePage, "/api/v1/books/{1}/pages", Post, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(PageController::listPages, "/api/v1/books/{1}/pages", Get, "JwtFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void savePage(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string bookId);
    void listPages(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   std::string bookId);
};
