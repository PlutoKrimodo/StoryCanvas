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

SaveBatchPagesRequest SaveBatchPagesRequest::fromJson(const json &j) {
    SaveBatchPagesRequest req;
    if (j.is_object() && j.contains("image_ids") && j["image_ids"].is_array()) {
        for (const auto &item : j["image_ids"]) {
            if (item.is_string()) {
                req.imageIds.push_back(item.get<std::string>());
            }
        }
    }
    return req;
}

std::vector<FieldError> SaveBatchPagesRequest::getErrors() const {
    std::vector<FieldError> errors;
    if (imageIds.empty() || imageIds.size() > 6) {
        errors.push_back({"image_ids", "一次需提交 1–6 张图片"});
        return errors;
    }
    for (const auto &id : imageIds) {
        if (id.empty()) {
            errors.push_back({"image_ids", "图片 ID 不能为空"});
            break;
        }
    }
    return errors;
}

bool SaveBatchPagesRequest::validate() const {
    return getErrors().empty();
}
