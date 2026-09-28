#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

class BookController : public drogon::HttpSimpleController<BookController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/books", Post, "JwtFilter");
    PATH_ADD("/api/v1/users/me/books", Get, "JwtFilter");
    PATH_ADD("/api/v1/books/{book_id}", Get, "JwtFilter");
    PATH_ADD("/api/v1/books/{book_id}", Put, "JwtFilter");
    PATH_ADD("/api/v1/books/{book_id}", Delete, "JwtFilter");
    PATH_LIST_END

private:
    void createBook(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void listMyBooks(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getBook(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateBook(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteBook(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
