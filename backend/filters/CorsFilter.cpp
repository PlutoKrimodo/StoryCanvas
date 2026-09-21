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
        cors::addCorsHeaders(resp, std::string(req->getHeader("Origin")));
        fcb(resp);
        return;
    }

    // 放行后由 post-handling 添加 CORS 头（已移除，改在此处放行）
    fccb();
}
