#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * BYOK 凭据接口（docs/16 §4.2 决策 7 扩展）。
 *
 * 全部挂 JwtFilter + MemberFilter：游客没有稳定的凭据归属，一律 403。
 * 路由必须用 HttpController + ADD_METHOD_TO，{1} 才能匹配 capability。
 *
 * 关闭 BYOK_ENABLED 时，服务层统一返回 404（接口视同不存在）。
 */
class UserAiCredentialController : public drogon::HttpController<UserAiCredentialController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserAiCredentialController::overview,
                  "/api/v1/users/me/ai-credentials", Get, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(UserAiCredentialController::providers,
                  "/api/v1/users/me/ai-credentials/providers", Get, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(UserAiCredentialController::save,
                  "/api/v1/users/me/ai-credentials/{1}", Put, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(UserAiCredentialController::remove,
                  "/api/v1/users/me/ai-credentials/{1}", Delete, "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(UserAiCredentialController::test,
                  "/api/v1/users/me/ai-credentials/test", Post, "JwtFilter", "MemberFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void overview(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

    void providers(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    void save(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback,
              std::string capability);

    void remove(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string capability);

    void test(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
};
