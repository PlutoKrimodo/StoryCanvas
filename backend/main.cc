#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include "filters/CorsFilter.h"
#include "filters/CorsHeaders.h"
#include "utils/ConfigManager.h"
#include "utils/JwtUtils.h"
#include "utils/PasswordUtils.h"

namespace {

trantor::Logger::LogLevel toLogLevel(const std::string &level) {
    if (level == "trace") return trantor::Logger::kTrace;
    if (level == "debug") return trantor::Logger::kDebug;
    if (level == "warn") return trantor::Logger::kWarn;
    if (level == "error") return trantor::Logger::kError;
    return trantor::Logger::kInfo;
}

}  // namespace

int main() {
    // 1. 加载配置：.env -> config/config.json -> 环境变量（后者优先）
    //    密钥类配置（JWT_SECRET 等）必须由环境变量注入，禁止硬编码
    ConfigManager::init();

    // 2. 初始化安全工具
    JwtUtils::init(ConfigManager::jwtSecret,
                   ConfigManager::jwtAccessTokenExpiry,
                   ConfigManager::jwtRefreshTokenExpiry);

    spdlog::set_level(spdlog::level::from_str(ConfigManager::logLevel));
    spdlog::info("密码哈希算法：{}", PasswordUtils::algorithmName());

    auto &app = drogon::app();

    // 3. 加载 Drogon 配置（端口、线程数等）
    app.loadConfigFile("./config/config.json");
    app.setLogLevel(toLogLevel(ConfigManager::logLevel));

    // 4. 全局跨域过滤器（仅处理 OPTIONS 预检）
    app.registerFilter(std::make_shared<CorsFilter>());

    // 5. 为所有响应统一补齐 CORS 头
    //    HttpFilter 无法修改下游响应，因此这里用后置 advice 实现
    app.registerPostHandlingAdvice(
        [](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
            cors::addCorsHeaders(resp, req->getHeader("Origin"));
        });

    // 6. Controller 通过 PATH_LIST_BEGIN / PATH_ADD 宏在编译期自动注册，
    //    无需在此手工登记（如 HealthController -> GET /health、/api/v1/health）

    spdlog::info("StoryCanvas Backend 启动中...");
    app.run();

    return 0;
}
