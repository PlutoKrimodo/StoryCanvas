#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

// 服务层统一返回值：所有 Service 方法均返回该结构
// success 表示业务是否成功；code 为建议的 HTTP 状态码；data 为响应数据（失败时为空）
struct ServiceResult {
    bool success = false;
    int code = 500;
    std::string message;
    json data;
};
