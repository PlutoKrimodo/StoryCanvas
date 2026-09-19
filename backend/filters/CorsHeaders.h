#pragma once

#include <drogon/HttpResponse.h>

#include <string>

#include "utils/ConfigManager.h"

namespace cors {

/**
 * 为响应写入统一 CORS 头。
 *
 * 预检响应（CorsFilter）与实际业务响应（main.cc 中的 post-handling advice）
 * 共用本函数，避免两处维护导致行为不一致。
 *
 * @param resp          待写入头的响应
 * @param requestOrigin 请求头中的 Origin，用于多来源时回显匹配项
 */
inline void addCorsHeaders(const drogon::HttpResponsePtr &resp,
                           const std::string &requestOrigin = "") {
    if (resp == nullptr) {
        return;
    }

    std::string allowOrigin = ConfigManager::corsOrigins;

    // 配置多个来源时（逗号分隔），回显与请求匹配的那一个
    if (allowOrigin.find(',') != std::string::npos && !requestOrigin.empty()) {
        size_t start = 0;
        while (start <= allowOrigin.size()) {
            const size_t pos = allowOrigin.find(',', start);
            std::string item = allowOrigin.substr(
                start, pos == std::string::npos ? std::string::npos : pos - start);

            const auto begin = item.find_first_not_of(" \t");
            const auto end = item.find_last_not_of(" \t");
            item = (begin == std::string::npos) ? "" : item.substr(begin, end - begin + 1);

            if (item == requestOrigin) {
                allowOrigin = requestOrigin;
                break;
            }
            if (pos == std::string::npos) {
                break;
            }
            start = pos + 1;
        }
    }

    resp->addHeader("Access-Control-Allow-Origin", allowOrigin);
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    resp->addHeader("Access-Control-Max-Age", "3600");
    resp->addHeader("Vary", "Origin");
}

}  // namespace cors
