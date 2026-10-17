#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/** 会话接口：登录用户持久化；游客不落库。 */
class ConversationController : public drogon::HttpController<ConversationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ConversationController::create, "/api/v1/conversations", Post,
                  "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(ConversationController::list, "/api/v1/conversations", Get,
                  "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(ConversationController::get, "/api/v1/conversations/{1}", Get,
                  "JwtFilter", "MemberFilter");
    ADD_METHOD_TO(ConversationController::remove, "/api/v1/conversations/{1}", Delete,
                  "JwtFilter", "MemberFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void create(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
    void list(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);
    void get(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback,
             std::string conversationId);
    void remove(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string conversationId);
};
