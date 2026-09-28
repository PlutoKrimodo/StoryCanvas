#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "AuthDto.h"  // 复用 FieldError

using json = nlohmann::json;

// 创建绘本请求（封面上传为加分项，本阶段不接受 cover_image）
struct CreateBookRequest {
    std::string title;
    std::string description;
    std::string status = "draft";

    static CreateBookRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

// 更新绘本请求（status 可选，缺省表示保持原状态）
struct UpdateBookRequest {
    std::string title;
    std::string description;
    std::string status;

    static UpdateBookRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};
