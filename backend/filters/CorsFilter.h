#pragma once

#include <drogon/HttpFilter.h>

using namespace drogon;

/**
 * 跨域过滤器（全局注册）
 *
 * 职责范围刻意收窄：
 *   - 仅负责 **OPTIONS 预检请求**的短路响应
 *   - 真实业务响应的 CORS 头由 main.cc 中的 post-handling advice 统一补齐
 *     （HttpFilter 无法修改下游响应，早期实现在此处加头会被丢弃）
 */
class CorsFilter : public drogon::HttpFilter<CorsFilter> {
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};
