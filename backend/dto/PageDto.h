#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "AuthDto.h"  // 复用 FieldError

using json = nlohmann::json;

/** 保存到绘本请求（创建/更新页并绑定生成图，docs/04 §5.1）。 */
struct SavePageRequest {
    int pageNumber = 0;
    std::string title;
    std::string imageId;

    static SavePageRequest fromJson(const json &j);
    bool validate() const;
    std::vector<FieldError> getErrors() const;
};
