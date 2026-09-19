#include "CorsFilter.h"

#include <drogon/HttpResponse.h>

#include "CorsHeaders.h"

void CorsFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) {
    // 预检请求直接返回 204，并带上 CORS 头
    if (req->getMethod() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k204NoContent);
        cors::addCorsHeaders(resp, req->getHeader("Origin"));
        fcb(resp);
        return;
    }

    // 其余请求放行；响应头由 post-handling advice 统一补齐
    fccb();
}
