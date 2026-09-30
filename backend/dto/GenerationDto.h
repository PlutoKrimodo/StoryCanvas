#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "AuthDto.h"  // 复用 FieldError

using json = nlohmann::json;

/** 创建生成任务请求（docs/04 §8.1）。 */
struct CreateGenerationRequest {
    std::string text;
    std::string bookId;   // 可选：绑定的绘本
    std::string pageId;   // 可选：绑定的页（页容器）
    std::string style = "cartoon";
    json parameters = json::object();

    static CreateGenerationRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};

/** 重新生成请求（docs/04 §8.5）。 */
struct RegenerateRequest {
    json parameters = json::object();

    static RegenerateRequest fromJson(const json &j);
    bool validate() const { return true; }
};
