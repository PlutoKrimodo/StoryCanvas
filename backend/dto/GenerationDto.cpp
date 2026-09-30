#include "GenerationDto.h"

namespace {

constexpr int kMaxTextLength = 2000;

bool isValidStyle(const std::string &style) {
    return style == "cartoon" || style == "watercolor" || style == "oil_painting" ||
           style == "anime" || style == "pencil_sketch";
}

}  // namespace

CreateGenerationRequest CreateGenerationRequest::fromJson(const json &j) {
    CreateGenerationRequest req;
    if (j.contains("text") && j["text"].is_string()) {
        req.text = j["text"].get<std::string>();
    }
    if (j.contains("book_id") && j["book_id"].is_string()) {
        req.bookId = j["book_id"].get<std::string>();
    }
    if (j.contains("page_id") && j["page_id"].is_string()) {
        req.pageId = j["page_id"].get<std::string>();
    }
    if (j.contains("style") && j["style"].is_string()) {
        req.style = j["style"].get<std::string>();
    }
    if (req.style.empty()) {
        req.style = "cartoon";
    }
    if (j.contains("parameters") && j["parameters"].is_object()) {
        req.parameters = j["parameters"];
    }
    return req;
}

std::vector<FieldError> CreateGenerationRequest::getErrors() const {
    std::vector<FieldError> errors;

    if (text.empty()) {
        errors.push_back({"text", "故事文本不能为空"});
    } else if (static_cast<int>(text.size()) > kMaxTextLength) {
        errors.push_back({"text", "故事文本不能超过 2000 个字符"});
    }

    if (!isValidStyle(style)) {
        errors.push_back(
            {"style", "风格必须是 cartoon/watercolor/oil_painting/anime/pencil_sketch 之一"});
    }

    return errors;
}

bool CreateGenerationRequest::validate() const {
    return getErrors().empty();
}

RegenerateRequest RegenerateRequest::fromJson(const json &j) {
    RegenerateRequest req;
    if (j.is_object() && j.contains("parameters") && j["parameters"].is_object()) {
        req.parameters = j["parameters"];
    }
    return req;
}
