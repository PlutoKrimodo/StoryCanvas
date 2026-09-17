#include "ConfigManager.h"
#include <fstream>
#include <cstdlib>
#include <spdlog/spdlog.h>

std::string ConfigManager::dbHost = "localhost";
int ConfigManager::dbPort = 5432;
std::string ConfigManager::dbName = "storycanvas";
std::string ConfigManager::dbUser = "storycanvas";
std::string ConfigManager::dbPassword = "";

std::string ConfigManager::jwtSecret = "";
int ConfigManager::jwtAccessTokenExpiry = 3600;
int ConfigManager::jwtRefreshTokenExpiry = 604800;

std::string ConfigManager::aiServiceUrl = "http://localhost:8000";
std::string ConfigManager::aiServiceApiKey = "";

std::string ConfigManager::storagePath = "./storage";

void ConfigManager::init() {
    auto getEnv = [](const std::string &key, const std::string &defaultVal) -> std::string {
        const char *val = std::getenv(key.c_str());
        return val ? std::string(val) : defaultVal;
    };

    dbHost = getEnv("DB_HOST", "localhost");
    dbPort = std::stoi(getEnv("DB_PORT", "5432"));
    dbName = getEnv("DB_NAME", "storycanvas");
    dbUser = getEnv("DB_USER", "storycanvas");
    dbPassword = getEnv("DB_PASSWORD", "");

    jwtSecret = getEnv("JWT_SECRET", "");
    jwtAccessTokenExpiry = std::stoi(getEnv("JWT_ACCESS_TOKEN_EXPIRY", "3600"));
    jwtRefreshTokenExpiry = std::stoi(getEnv("JWT_REFRESH_TOKEN_EXPIRY", "604800"));

    aiServiceUrl = getEnv("AI_SERVICE_URL", "http://localhost:8000");
    aiServiceApiKey = getEnv("AI_SERVICE_API_KEY", "");

    storagePath = getEnv("STORAGE_PATH", "./storage");

    spdlog::info("ConfigManager initialized");
    spdlog::info("DB: {}:{}/{}", dbHost, dbPort, dbName);
    spdlog::info("AI Service: {}", aiServiceUrl);
}
