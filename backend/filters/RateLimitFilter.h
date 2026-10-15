#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

/**
 * 全局限流（Q5 落地，规格见 docs/16 §7.4，设计见 docs/10 §4.1）。
 *
 * 策略（固定窗口 60s，计数见 utils/RateLimiter）：
 *   - 匿名（无有效 Access Token）按客户端 IP：默认 100 次/分
 *   - 认证（有效 Access Token，含游客）按用户：默认 1000 次/分
 *   - AI 生成（创建 / 重新生成）按身份：默认 10 次/分
 *   - 游客会话签发（POST /auth/guest）按 IP：默认 10 次/分（Q3①，防无限换取游客身份）
 *
 * 接入方式说明：Drogon 1.9 的「全局过滤器」需以 advice 形式运行（libdrogon 的
 * GlobalFilters 插件即如此实现），故本类不继承 HttpFilter，而是在 main.cpp 中
 * 通过 registerGlobalAdvice() 注册同步 advice，覆盖所有请求（含未匹配路由）。
 *
 * 例外（豁免限流）：OPTIONS 预检、/health 与 /api/v1/health、
 * 生成图 raw（`<img>` 标签直连加载，无 Authorization 头，且资源为只读）。
 */
class RateLimitFilter {
public:
    /**
     * 注册全局同步 advice（应在 app().run() 之前调用一次）。
     * 未启用（RATE_LIMIT_ENABLED=false）时仅记录日志，不注册。
     */
    static void registerGlobalAdvice();

private:
    /** 检查请求并消费额度；返回非空响应表示应拦截（429），nullptr 表示放行 */
    static drogon::HttpResponsePtr check(const drogon::HttpRequestPtr &req);
};
