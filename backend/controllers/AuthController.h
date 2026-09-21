#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

class AuthController : public drogon::HttpSimpleController<AuthController> {
public: 
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/auth/register", Post);
    PATH_ADD("/api/v1/auth/login", Post);
    PATH_ADD("/api/v1/auth/refresh", Post);
    PATH_ADD("/api/v1/auth/logout", Post, "JwtFilter");
    PATH_LIST_END

private:
    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void refreshToken(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void logout(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};