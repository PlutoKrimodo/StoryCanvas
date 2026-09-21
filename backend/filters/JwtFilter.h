#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

class JwtFilter : public drogon::HttpFilter<JwtFilter> {
public:
    void doFilter(const HttpRequestPtr &req, FilterCallback &&fcb, FilterChainCallback &&fccb)override;
};