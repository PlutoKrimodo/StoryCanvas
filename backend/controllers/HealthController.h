#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

/**
 * 健康检查
 *
 * 同时暴露两个路径：
 *   - /health        基础设施探活（不带版本前缀，供容器 healthcheck 使用）
 *   - /api/v1/health 对外统一 API 前缀（与 docs/04 的接口契约对齐）
 */
class HealthController : public drogon::HttpSimpleController<HealthController> {
public:
    void asyncHandleHttpRequest(
        const HttpRequestPtr &req,
        std::function<void(const HttpResponsePtr &)> &&callback) override;

    static void init() {}
    static void shutdown() {}

    PATH_LIST_BEGIN
    PATH_ADD("/health", Get);
    PATH_ADD("/api/v1/health", Get);
    PATH_LIST_END
};
