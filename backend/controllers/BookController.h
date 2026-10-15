#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * 绘本接口（docs/04 §4）。
 *
 * 路由注册必须用 HttpController + ADD_METHOD_TO（原因见 GenerationController.h 注释）：
 * HttpSimpleController 的 PATH_ADD 只做精确路径匹配，{book_id} 不会命中真实请求。
 * {1} 必须对应 handler 的一个参数，否则启动即 exit(1)。
 *
 * 写接口额外挂 MemberFilter（Q3②）：游客只读不写，创建 / 更新 / 删除绘本一律 403。
 */
class BookController : public drogon::HttpController<BookController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BookController::createBook, "/api/v1/books", Post, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(BookController::listMyBooks, "/api/v1/users/me/books", Get, "JwtFilter");
    ADD_METHOD_TO(BookController::getBook, "/api/v1/books/{1}", Get, "JwtFilter");
    ADD_METHOD_TO(BookController::updateBook, "/api/v1/books/{1}", Put, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(BookController::deleteBook, "/api/v1/books/{1}", Delete, "JwtFilter", "MemberFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void createBook(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    void listMyBooks(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
    void getBook(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 std::string bookId);
    void updateBook(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string bookId);
    void deleteBook(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string bookId);
};
