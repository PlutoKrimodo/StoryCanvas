#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * 生成任务接口（docs/04 §8）。
 *
 * 路由注册必须用 HttpController + ADD_METHOD_TO，不能用 HttpSimpleController + PATH_ADD：
 * Drogon 的简单控制器只做「精确路径」哈希匹配（HttpControllersRouter::route 中
 * simpleCtrlMap_.find()，没有正则回退），因此 PATH_ADD("/api/v1/generation/{task_id}")
 * 注册进去的是字面量 key，永远匹配不到真实请求 /api/v1/generation/<uuid>，会直接 404。
 * 只有 HttpController 的 ADD_METHOD_TO 才会把 {1} 展开为正则 ([^/]*)。
 *
 * 注意：{1} 必须对应 handler 的一个参数，否则 Drogon 在启动时会 exit(1)。
 */
class GenerationController : public drogon::HttpController<GenerationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GenerationController::createTask, "/api/v1/generation", Post, "JwtFilter");
    ADD_METHOD_TO(GenerationController::listHistory, "/api/v1/generation/history", Get, "JwtFilter");
    ADD_METHOD_TO(GenerationController::getTask, "/api/v1/generation/{1}", Get, "JwtFilter");
    ADD_METHOD_TO(GenerationController::cancelTask, "/api/v1/generation/{1}/cancel", Post, "JwtFilter");
    ADD_METHOD_TO(GenerationController::regenerate, "/api/v1/generation/{1}/regenerate", Post, "JwtFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void createTask(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    void listHistory(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
    void getTask(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback,
                 std::string taskId);
    void cancelTask(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string taskId);
    void regenerate(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string taskId);
};
