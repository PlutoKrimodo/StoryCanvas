#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/**
 * 统一响应结构：{ code, message, data }
 *
 * 约定（见 docs/16-项目实施方案.md 第 7.1 节）：
 *   - 无论成功或失败，响应体都包含 data 字段
 *   - 失败时 data 必须补齐为 null，便于前端统一处理
 */
class ApiResponse {
public:
    static json success(const std::string &message) {
        return {
            {"code", 200},
            {"message", message},
            {"data", nullptr}
        };
    }

    static json success(const std::string &message, const json &data) {
        return {
            {"code", 200},
            {"message", message},
            {"data", data}
        };
    }

    static json created(const std::string &message, const json &data) {
        return {
            {"code", 201},
            {"message", message},
            {"data", data}
        };
    }

    static json error(int code, const std::string &message) {
        return {
            {"code", code},
            {"message", message},
            {"data", nullptr}
        };
    }

    /** 字段级校验失败：errors 为 [{ field, message }] */
    static json validationError(const std::string &message, const json &errors) {
        return {
            {"code", 422},
            {"message", message},
            {"data", nullptr},
            {"errors", errors}
        };
    }
};
