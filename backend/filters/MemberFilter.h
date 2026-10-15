#pragma once

#include <drogon/HttpFilter.h>

/**
 * 正式用户过滤器（Q3②）：
 * 拒绝游客 Token 调用「写」接口——创建 / 更新 / 删除绘本、保存页。
 * 产品口径见 docs/18 §2「游客不保存绘本」；此前只有前端拦截，后端可被绕过。
 *
 * 必须挂在 JwtFilter **之后**（依赖其写入请求属性的 `is_guest`）。
 * 非法 Token 在 JwtFilter 即被 401 拦截，本过滤器只面对已通过认证的请求。
 */
class MemberFilter : public drogon::HttpFilter<MemberFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};
