#include "MemberFilter.h"

#include "../dto/ApiResponse.h"

void MemberFilter::doFilter(const drogon::HttpRequestPtr &req,
                            drogon::FilterCallback &&fcb,
                            drogon::FilterChainCallback &&fccb) {
    // is_guest 由 JwtFilter 写入；缺失时按正式用户处理（路径注册错误才会发生）
    const bool isGuest = req->getAttributes()->get<bool>("is_guest");
    if (isGuest) {
        fcb(ApiResponse::fail(403, "游客无法使用该功能，请注册或登录后再试"));
        return;
    }
    fccb();
}
