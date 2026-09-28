#include "BookDto.h"

namespace {

bool isValidStatus(const std::string &status) {
    return status == "draft" || status == "published" || status == "archived";
}

}  // namespace

CreateBookRequest CreateBookRequest::fromJson(const json &j) {
    CreateBookRequest req;
    if (j.contains("title") && j["title"].is_string()) {
        req.title = j["title"].get<std::string>();
    }
    if (j.contains("description") && j["description"].is_string()) {
        req.description = j["description"].get<std::string>();
    }
    if (j.contains("status") && j["status"].is_string()) {
        req.status = j["status"].get<std::string>();
    }
    if (req.status.empty()) {
        req.status = "draft";
    }
    return req;
}

std::vector<FieldError> CreateBookRequest::getErrors() const {
    std::vector<FieldError> errors;

    if (title.empty()) {
        errors.push_back({"title", "标题不能为空"});
    } else if (title.size() > 200) {
        errors.push_back({"title", "标题不能超过 200 个字符"});
    }

    if (description.size() > 2000) {
        errors.push_back({"description", "描述不能超过 2000 个字符"});
    }

    if (!isValidStatus(status)) {
        errors.push_back({"status", "状态必须是 draft/published/archived 之一"});
    }

    return errors;
}

bool CreateBookRequest::validate() const {
    return getErrors().empty();
}

UpdateBookRequest UpdateBookRequest::fromJson(const json &j) {
    UpdateBookRequest req;
    if (j.contains("title") && j["title"].is_string()) {
        req.title = j["title"].get<std::string>();
    }
    if (j.contains("description") && j["description"].is_string()) {
        req.description = j["description"].get<std::string>();
    }
    if (j.contains("status") && j["status"].is_string()) {
        req.status = j["status"].get<std::string>();
    }
    return req;
}

std::vector<FieldError> UpdateBookRequest::getErrors() const {
    std::vector<FieldError> errors;

    if (title.empty()) {
        errors.push_back({"title", "标题不能为空"});
    } else if (title.size() > 200) {
        errors.push_back({"title", "标题不能超过 200 个字符"});
    }

    if (description.size() > 2000) {
        errors.push_back({"description", "描述不能超过 2000 个字符"});
    }

    // 更新时 status 允许缺省（保持原状态），仅在提供时校验枚举
    if (!status.empty() && !isValidStatus(status)) {
        errors.push_back({"status", "状态必须是 draft/published/archived 之一"});
    }

    return errors;
}

bool UpdateBookRequest::validate() const {
    return getErrors().empty();
}
