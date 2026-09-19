#include "ConfigManager.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

nlohmann::json ConfigManager::fileConfig_ = nlohmann::json::object();

std::string ConfigManager::dbHost = "localhost";
int ConfigManager::dbPort = 5432;
std::string ConfigManager::dbName = "storycanvas";
std::string ConfigManager::dbUser = "storycanvas";
std::string ConfigManager::dbPassword;

std::string ConfigManager::jwtSecret;
int ConfigManager::jwtAccessTokenExpiry = 3600;
int ConfigManager::jwtRefreshTokenExpiry = 604800;

std::string ConfigManager::aiServiceUrl = "http://localhost:8000";
std::string ConfigManager::aiServiceApiKey;

std::string ConfigManager::corsOrigins = "http://localhost:5173";
std::string ConfigManager::encryptionKey;
std::string ConfigManager::logLevel = "info";
std::string ConfigManager::storagePath = "./storage";

namespace {

/// .env 文件中读取到的键值（仅用于填补未设置的真实环境变量）
std::unordered_map<std::string, std::string> g_dotEnv;

std::string trim(const std::string &text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string stripQuotes(const std::string &text) {
    if (text.size() >= 2) {
        const char first = text.front();
        const char last = text.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return text.substr(1, text.size() - 2);
        }
    }
    return text;
}

/** 真实环境变量优先，其次 .env 文件 */
std::string lookup(const std::string &key) {
    if (const char *value = std::getenv(key.c_str()); value != nullptr && *value != '\0') {
        return std::string(value);
    }
    const auto it = g_dotEnv.find(key);
    return it != g_dotEnv.end() ? it->second : std::string();
}

}  // namespace

void ConfigManager::loadDotEnv(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::debug("未找到 .env 文件（{}），跳过", path);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const auto pos = trimmed.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = trim(trimmed.substr(0, pos));
        const std::string value = stripQuotes(trim(trimmed.substr(pos + 1)));
        if (key.empty()) {
            continue;
        }

        // 真实环境变量优先级更高，已设置则不覆盖
        if (const char *existing = std::getenv(key.c_str()); existing != nullptr && *existing != '\0') {
            continue;
        }
        g_dotEnv[key] = value;
    }

    spdlog::debug(".env 已加载：{} 项", g_dotEnv.size());
}

void ConfigManager::loadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::warn("未找到配置文件 {}，将仅使用环境变量", path);
        return;
    }

    try {
        file >> fileConfig_;
    } catch (const std::exception &ex) {
        fileConfig_ = nlohmann::json::object();
        spdlog::warn("解析配置文件失败：{}", ex.what());
    }
}

void ConfigManager::applyEnv() {
    auto fromFile = [](const char *section, const char *key) -> std::string {
        try {
            if (!fileConfig_.contains(section)) {
                return "";
            }
            const auto &node = fileConfig_.at(section);
            if (!node.is_object() || !node.contains(key)) {
                return "";
            }
            const auto &value = node.at(key);
            if (value.is_string()) {
                return value.get<std::string>();
            }
            if (value.is_number_integer()) {
                return std::to_string(value.get<long long>());
            }
        } catch (const std::exception &) {
        }
        return "";
    };

    // 环境变量优先，其次 config.json（config.json 中不放敏感值）
    auto resolve = [&fromFile](const char *envKey, const char *section, const char *jsonKey,
                               const std::string &fallback) -> std::string {
        const std::string fromEnv = lookup(envKey);
        if (!fromEnv.empty()) {
            return fromEnv;
        }
        const std::string fromJson = fromFile(section, jsonKey);
        return fromJson.empty() ? fallback : fromJson;
    };

    auto toInt = [](const std::string &text, int fallback) -> int {
        try {
            return std::stoi(text);
        } catch (const std::exception &) {
            return fallback;
        }
    };

    dbHost = resolve("DB_HOST", "database", "host", "localhost");
    dbPort = toInt(resolve("DB_PORT", "database", "port", "5432"), 5432);
    dbName = resolve("DB_NAME", "database", "database", "storycanvas");
    dbUser = resolve("DB_USER", "database", "username", "storycanvas");
    dbPassword = resolve("DB_PASSWORD", "database", "password", "");

    jwtSecret = resolve("JWT_SECRET", "jwt", "secret", "");
    jwtAccessTokenExpiry =
        toInt(resolve("JWT_ACCESS_TOKEN_EXPIRY", "jwt", "access_token_expiry", "3600"), 3600);
    jwtRefreshTokenExpiry =
        toInt(resolve("JWT_REFRESH_TOKEN_EXPIRY", "jwt", "refresh_token_expiry", "604800"), 604800);

    aiServiceUrl = resolve("AI_SERVICE_URL", "ai_service", "url", "http://localhost:8000");
    aiServiceApiKey = resolve("AI_SERVICE_API_KEY", "ai_service", "api_key", "");

    corsOrigins = resolve("CORS_ORIGINS", "cors", "allowed_origins", "http://localhost:5173");
    encryptionKey = resolve("ENCRYPTION_KEY", "security", "encryption_key", "");
    logLevel = resolve("LOG_LEVEL", "app", "log_level", "info");
    storagePath = resolve("STORAGE_PATH", "storage", "local_path", "./storage");
}

void ConfigManager::validate() {
    if (jwtSecret.empty()) {
        spdlog::critical("JWT_SECRET 未配置：请在环境变量或 .env 中注入（禁止硬编码）");
        std::exit(EXIT_FAILURE);
    }
    if (aiServiceApiKey.empty()) {
        spdlog::warn("AI_SERVICE_API_KEY 未配置，后端与 AI 服务的内部调用将无法鉴权");
    }
    if (dbPassword.empty()) {
        spdlog::warn("DB_PASSWORD 未配置，数据库连接可能失败");
    }
    if (encryptionKey.empty()) {
        spdlog::debug("ENCRYPTION_KEY 未配置：未启用敏感字段 AES 加密存储");
    }
}

void ConfigManager::init() {
    loadDotEnv(".env");
    loadFromFile("./config/config.json");
    applyEnv();
    validate();

    spdlog::info("配置加载完成 | DB {}:{}/{} | AI Service {} | CORS {} | 日志级别 {}",
                 dbHost, dbPort, dbName, aiServiceUrl, corsOrigins, logLevel);
}
