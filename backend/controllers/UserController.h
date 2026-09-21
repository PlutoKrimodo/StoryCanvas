#pragma once

#include <drogon/HttpSimpleController.h>
using namespace drogon;

class UserController : public drogon::HttpSimpleController<UserController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/users/me", Get, "JwtFilter");
    PATH_ADD("/api/v1/users/me", Put, "JwtFilter");
    PATH_ADD("/api/v1/users/me/password", Put, "JwtFilter");
    PATH_LIST_END
private:
    void getMe(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateMe(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void changePassword(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};