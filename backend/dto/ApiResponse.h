#pragma once
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class ApiResponse {
public:
    static json success(const std::string &message) {
        return {
            {"code", 200},
            {"message", message}
        };
    }

    static json success(const std::string &message, const json &data) {
        return {
            {"code", 200},
            {"message", message},
            {"data", data}
        };
    }

    static json error(int code, const std::string &message) {
        return {
            {"code", code},
            {"message", message}
        };
    }
};
