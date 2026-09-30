#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

/** 生成任务接口（docs/04 §8）。 */
class GenerationController : public drogon::HttpSimpleController<GenerationController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/api/v1/generation", Post, "JwtFilter");
    PATH_ADD("/api/v1/generation/history", Get, "JwtFilter");
    PATH_ADD("/api/v1/generation/{task_id}", Get, "JwtFilter");
    PATH_ADD("/api/v1/generation/{task_id}/cancel", Post, "JwtFilter");
    PATH_ADD("/api/v1/generation/{task_id}/regenerate", Post, "JwtFilter");
    PATH_LIST_END

private:
    void createTask(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getTask(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void listHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void cancelTask(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void regenerate(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
