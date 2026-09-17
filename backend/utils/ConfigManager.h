#pragma once
#include <string>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
    static void init();
    static void loadFromFile(const std::string &path);

    static std::string dbHost;
    static int dbPort;
    static std::string dbName;
    static std::string dbUser;
    static std::string dbPassword;

    static std::string jwtSecret;
    static int jwtAccessTokenExpiry;
    static int jwtRefreshTokenExpiry;

    static std::string aiServiceUrl;
    static std::string aiServiceApiKey;

    static std::string storagePath;
};
