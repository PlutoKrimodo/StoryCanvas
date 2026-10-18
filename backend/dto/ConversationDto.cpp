#include "ConversationDto.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace {

constexpr int kMaxPanels = 12;
constexpr int kMaxTextLength = 2000;

const std::set<std::string> kStyles = {
    "cartoon", "watercolor", "oil_painting", "anime", "pencil_sketch"
};

std::string trim(const std::string &text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

bool validStyle(const std::string &style) {
    return kStyles.count(style) > 0;
}

bool validSize(const std::string &size) {
    if (size.empty() || size.size() > 20) return false;
    const auto pos = size.find('x');
    if (pos == std::string::npos || pos != size.rfind('x')) return false;
    if (pos == 0 || pos + 1 >= size.size()) return false;
    for (char ch : size) {
        if (ch != 'x' && !std::isdigit(static_cast<unsigned char>(ch))) return false;
    }
    return true;
}

std::string stringField(const json &body, const char *key) {
    if (!body.is_object() || !body.contains(key)) return "";
    const auto &value = body.at(key);
    return value.is_string() ? value.get<std::string>() : "";
}

}  // namespace

CreatePlanRequest CreatePlanRequest::fromJson(const json &j) {
    CreatePlanRequest request;
    request.conversationId = stringField(j, "conversation_id");
    request.text = stringField(j, "text");
    request.style = stringField(j, "style");
    if (request.style.empty()) request.style = "cartoon";
    request.size = stringField(j, "size");
    if (request.size.empty()) request.size = "1024x1024";
    if (j.is_object() && j.contains("image_count") && j["image_count"].is_number_integer()) {
        request.imageCount = j["image_count"].get<int>();
    }
    return request;
}

bool CreatePlanRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> CreatePlanRequest::getErrors() const {
    std::vector<FieldError> errors;
    const std::string value = trim(text);
    if (value.empty()) {
        errors.push_back({"text", "请输入故事描述"});
    } else if (value.size() > kMaxTextLength) {
        errors.push_back({"text", "故事描述不能超过 2000 个字符"});
    }
    if (imageCount < 1 || imageCount > kMaxPanels) {
        errors.push_back({"image_count", "生成张数需在 1–12 之间"});
    }
    if (!validStyle(style)) {
        errors.push_back({"style", "不支持的艺术风格"});
    }
    if (!validSize(size)) {
        errors.push_back({"size", "尺寸格式应为 1024x1024"});
    }
    return errors;
}

UpdatePlanRequest UpdatePlanRequest::fromJson(const json &j) {
    UpdatePlanRequest request;
    if (!j.is_object() || !j.contains("panels") || !j["panels"].is_array()) {
        return request;
    }
    for (const auto &item : j["panels"]) {
        if (!item.is_object()) continue;
        PlanPanelInput panel;
        if (item.contains("index") && item["index"].is_number_integer()) {
            panel.index = item["index"].get<int>();
        }
        panel.text = stringField(item, "text");
        request.panels.push_back(panel);
    }
    return request;
}

bool UpdatePlanRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> UpdatePlanRequest::getErrors() const {
    std::vector<FieldError> errors;
    if (panels.empty() || panels.size() > kMaxPanels) {
        errors.push_back({"panels", "分镜数量需在 1–12 之间"});
        return errors;
    }
    for (size_t i = 0; i < panels.size(); ++i) {
        const auto &panel = panels[i];
        if (panel.index != static_cast<int>(i)) {
            errors.push_back({"panels", "分镜序号必须从 0 连续递增"});
            break;
        }
        const std::string value = trim(panel.text);
        if (value.empty() || value.size() > 500) {
            errors.push_back({"panels", "每格描述需为 1–500 个字符"});
            break;
        }
    }
    return errors;
}

json UpdatePlanRequest::toJson() const {
    json items = json::array();
    for (const auto &panel : panels) {
        items.push_back({{"index", panel.index}, {"text", trim(panel.text)}});
    }
    return items;
}

StartBatchRequest StartBatchRequest::fromJson(const json &j) {
    StartBatchRequest request;
    if (!j.is_object()) return request;
    if (j.contains("seed") && j["seed"].is_number_integer()) {
        request.seed = j["seed"].get<int>();
    }
    request.bookId = stringField(j, "book_id");
    request.style = stringField(j, "style");
    request.size = stringField(j, "size");
    return request;
}

bool StartBatchRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> StartBatchRequest::getErrors() const {
    std::vector<FieldError> errors;
    if (!style.empty() && !validStyle(style)) {
        errors.push_back({"style", "不支持的艺术风格"});
    }
    if (!size.empty() && !validSize(size)) {
        errors.push_back({"size", "尺寸格式应为 1024x1024"});
    }
    if (seed > 2147483647) {
        errors.push_back({"seed", "随机种子超出范围"});
    }
    return errors;
}

CreateConversationRequest CreateConversationRequest::fromJson(const json &j) {
    CreateConversationRequest request;
    request.title = stringField(j, "title");
    request.style = stringField(j, "style");
    request.size = stringField(j, "size");
    return request;
}

bool CreateConversationRequest::validate() const {
    return getErrors().empty();
}

std::vector<FieldError> CreateConversationRequest::getErrors() const {
    std::vector<FieldError> errors;
    if (trim(title).size() > 120) {
        errors.push_back({"title", "标题不能超过 120 个字符"});
    }
    if (!style.empty() && !validStyle(style)) {
        errors.push_back({"style", "不支持的艺术风格"});
    }
    if (!size.empty() && !validSize(size)) {
        errors.push_back({"size", "尺寸格式应为 1024x1024"});
    }
    return errors;
}
