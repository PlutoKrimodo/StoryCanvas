#include "PageDto.h"

namespace {
constexpr int kMaxPageNumber = 200;
constexpr int kMaxTitleLength = 200;
}  // namespace

SavePageRequest SavePageRequest::fromJson(const json &j) {
    SavePageRequest req;
    if (j.contains("page_number") && j["page_number"].is_number_integer()) {
        req.pageNumber = j["page_number"].get<int>();
    }
    if (j.contains("title") && j["title"].is_string()) {
        req.title = j["title"].get<std::string>();
    }
    if (j.contains("image_id") && j["image_id"].is_string()) {
        req.imageId = j["image_id"].get<std::string>();
    }
    return req;
}

std::vector<FieldError> SavePageRequest::getErrors() const {
    std::vector<FieldError> errors;

    if (pageNumber < 1) {
        errors.push_back({"page_number", "页码必须从 1 开始"});
    } else if (pageNumber > kMaxPageNumber) {
        errors.push_back({"page_number", "页码不能超过 200"});
    }

    if (imageId.empty()) {
        errors.push_back({"image_id", "必须绑定一张生成图"});
    }

    if (static_cast<int>(title.size()) > kMaxTitleLength) {
        errors.push_back({"title", "页标题不能超过 200 个字符"});
    }

    return errors;
}

bool SavePageRequest::validate() const {
    return getErrors().empty();
}
