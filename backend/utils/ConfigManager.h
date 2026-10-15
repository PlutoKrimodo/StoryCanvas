#pragma once

#include <nlohmann/json.hpp>
#include <string>

/**
 * 配置管理
 *
 * 加载顺序（后者覆盖前者）：
 *   1. config/config.json —— 仅提供**非敏感**默认值
 *   2. .env 文件          —— 本地开发便利（仅填补未设置的环境变量）
 *   3. 进程环境变量        —— 最高优先级，**密钥类必须由此注入**
 *
 * 约束：密码、JWT 密钥、第三方 API Key 一律不得写入代码或 config.json。
 */
class ConfigManager {
public:
    static void init();

    // 数据库
    static std::string dbHost;
    static int dbPort;
    static std::string dbName;
    static std::string dbUser;
    static std::string dbPassword;

    // JWT
    static std::string jwtSecret;
    static int jwtAccessTokenExpiry;
    static int jwtRefreshTokenExpiry;
    static int jwtGuestTokenExpiry;  // 游客会话 Token 寿命（秒），需长于普通 Access

    // Guest-first（T-1 能力层）
    static int guestGenerationQuota;  // 游客可免费生成的图片数（默认 1）

    // 全站成本封顶：今天所有用户合计可创建的任务数上限
    static int globalDailyGenerationCap;

    // 接口限流（Q5 / Q3①，规格见 docs/16 §7.4，实现在 filters/RateLimitFilter）
    static bool rateLimitEnabled;             // 总开关（开发 / 压测可关）
    static int rateLimitAnonPerMin;           // 匿名（按 IP）每分钟上限
    static int rateLimitAuthPerMin;           // 认证（按用户）每分钟上限
    static int rateLimitGenerationPerMin;     // AI 生成接口（创建 / 重新生成）每分钟上限
    static int rateLimitGuestTokenPerMin;     // 游客会话签发（按 IP）每分钟上限

    // AI Service
    static std::string aiServiceUrl;
    static std::string aiServiceApiKey;
    static int aiServiceTimeout;  // 单次 AI 调用超时（秒），图像生成为长耗时操作

    // 其他
    static std::string corsOrigins;    // 逗号分隔的允许来源
    static std::string encryptionKey;  // AES-256 密钥（Base64），未启用按需加密时可为空
    static std::string logLevel;       // trace/debug/info/warn/error
    static std::string storagePath;

private:
    static void loadDotEnv(const std::string &path);
    static void loadFromFile(const std::string &path);
    static void applyEnv();
    static void validate();

    static nlohmann::json fileConfig_;
};
