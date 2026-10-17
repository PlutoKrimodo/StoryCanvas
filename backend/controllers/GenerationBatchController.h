#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

/**
 * 对话式多图生成接口。
 *
 * 游客可用（受 1 张额度限制），因此不挂 MemberFilter；
 * 会话本身仍只对登录用户开放。
 */
class GenerationBatchController : public drogon::HttpController<GenerationBatchController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GenerationBatchController::createPlan, "/api/v1/generation/plans", Post,
                  "JwtFilter");
    ADD_METHOD_TO(GenerationBatchController::updatePlan, "/api/v1/generation/plans/{1}", Put,
                  "JwtFilter");
    ADD_METHOD_TO(GenerationBatchController::startBatch,
                  "/api/v1/generation/batches/{1}/start", Post, "JwtFilter");
    ADD_METHOD_TO(GenerationBatchController::getBatch,
                  "/api/v1/generation/batches/{1}", Get, "JwtFilter");
    ADD_METHOD_TO(GenerationBatchController::cancelBatch,
                  "/api/v1/generation/batches/{1}/cancel", Post, "JwtFilter");
    ADD_METHOD_TO(GenerationBatchController::regeneratePanel,
                  "/api/v1/generation/batches/{1}/panels/{2}/regenerate", Post, "JwtFilter");
    METHOD_LIST_END

    static void init() {}
    static void shutdown() {}

    void createPlan(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
    void updatePlan(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string batchId);
    void startBatch(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    std::string batchId);
    void getBatch(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  std::string batchId);
    void cancelBatch(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     std::string batchId);
    void regeneratePanel(const HttpRequestPtr &req,
                         std::function<void(const HttpResponsePtr &)> &&callback,
                         std::string batchId,
                         std::string panelIndex);
};
