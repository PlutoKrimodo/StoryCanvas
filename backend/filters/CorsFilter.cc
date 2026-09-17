#include "CorsFilter.h"
#include <drogon/HttpResponse.h>

void CorsFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) {
    auto resp = HttpResponse::newHttpResponse();

    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    resp->addHeader("Access-Control-Max-Age", "3600");

    if (req->getMethod() == Options) {
        resp->setStatusCode(k204NoContent);
        fcb(resp);
        return;
    }

    fccb();
}
