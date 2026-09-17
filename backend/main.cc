#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include "utils/ConfigManager.h"

int main() {
    // 初始化配置
    ConfigManager::init();
    
    // 配置 Drogon
    auto &app = drogon::app();
    
    // 加载配置文件
    app.loadConfigFile("./config/config.json");
    
    // 注册路由
    // ... 路由注册代码
    
    // 启动应用
    spdlog::info("StoryCanvas Backend 启动中...");
    app.run();
    
    return 0;
}