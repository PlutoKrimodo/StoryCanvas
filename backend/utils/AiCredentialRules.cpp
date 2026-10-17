#include "AiCredentialRules.h"

#include <algorithm>
#include <cctype>

namespace {

using json = nlohmann::json;

const std::vector<AiProviderPreset> kLlmProviders = {
    {"deepseek", "DeepSeek", "性价比高，中文理解稳定",
     "https://api.deepseek.com", "https://platform.deepseek.com/api_keys", false,
     {{"deepseek-chat", "DeepSeek Chat"}, {"deepseek-reasoner", "DeepSeek Reasoner"}}},
    {"openai", "OpenAI", "GPT 系列，适合复杂叙事",
     "https://api.openai.com/v1", "https://platform.openai.com/api-keys", false,
     {{"gpt-4o-mini", "GPT-4o mini"}, {"gpt-4o", "GPT-4o"}, {"gpt-4.1-mini", "GPT-4.1 mini"}}},
    {"dashscope", "通义千问", "阿里云百炼，国内直连",
     "https://dashscope.aliyuncs.com/compatible-mode/v1",
     "https://bailian.console.aliyun.com/", false,
     {{"qwen-plus", "Qwen Plus"}, {"qwen-turbo", "Qwen Turbo"}, {"qwen-max", "Qwen Max"}}},
    {"zhipu", "智谱 GLM", "国内直连，免费额度较多",
     "https://open.bigmodel.cn/api/paas/v4", "https://open.bigmodel.cn/usercenter/apikeys",
     false,
     {{"glm-4-flash", "GLM-4 Flash"}, {"glm-4-air", "GLM-4 Air"}, {"glm-4-plus", "GLM-4 Plus"}}},
    {"custom", "自定义（OpenAI 兼容）", "任何兼容 /chat/completions 的服务",
     "", "", true,
     {}},
};

const std::vector<AiProviderPreset> kImageProviders = {
    {"doubao", "豆包 · Seedream", "火山方舟，画面质感好",
     "https://ark.cn-beijing.volces.com/api/v3",
     "https://console.volcengine.com/ark", false,
     {{"doubao-seedream-5-0-flash-260915", "Seedream 5.0 Flash"},
      {"doubao-seedream-4-0-250828", "Seedream 4.0"}}},
    {"wanx", "通义万相", "阿里云百炼，国内直连",
     "https://dashscope.aliyuncs.com", "https://bailian.console.aliyun.com/", false,
     {{"wan2.6-image", "万相 2.6"}, {"wanx2.1-t2i-turbo", "万相 2.1 Turbo"}}},
};

const std::vector<AiProviderPreset> kEmptyProviders;

std::string trim(const std::string &text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool isAsciiPrintable(const std::string &text) {
    for (unsigned char ch : text) {
        if (ch < 0x21 || ch > 0x7E) {  // 禁止空白与控制字符
            return false;
        }
    }
    return true;
}

bool parsePort(const std::string &port, bool &ok) {
    ok = port.empty() || port == "443";
    if (port.empty()) {
        return true;
    }
    for (unsigned char ch : port) {
        if (!std::isdigit(ch)) {
            ok = false;
            return false;
        }
    }
    return true;
}

bool isIpv4Literal(const std::string &host) {
    int parts = 0;
    std::size_t start = 0;
    while (start <= host.size()) {
        const std::size_t dot = host.find('.', start);
        const std::string segment =
            host.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (segment.empty() || segment.size() > 3) {
            return false;
        }
        for (unsigned char ch : segment) {
            if (!std::isdigit(ch)) {
                return false;
            }
        }
        const int value = std::stoi(segment);
        if (value < 0 || value > 255) {
            return false;
        }
        ++parts;
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts == 4;
}

bool isValidHost(const std::string &host) {
    if (host.empty() || host.size() > 253) {
        return false;
    }
    if (host == "localhost" ||
        (host.size() > 10 && host.compare(host.size() - 10, 10, ".localhost") == 0)) {
        return false;
    }
    if (host.front() == '.' || host.back() == '.' || host.find("..") != std::string::npos) {
        return false;
    }
    for (unsigned char ch : host) {
        if (!(std::isalnum(ch) || ch == '-' || ch == '.')) {
            return false;
        }
    }
    return host.find('.') != std::string::npos;  // 必须是完整域名，拒绝单段内网主机名
}

}  // namespace

bool AiCredentialRules::parseCapability(const std::string &text, AiCapability &capability) {
    if (text == "llm") {
        capability = AiCapability::Llm;
        return true;
    }
    if (text == "image") {
        capability = AiCapability::Image;
        return true;
    }
    capability = AiCapability::Unknown;
    return false;
}

std::string AiCredentialRules::capabilityName(AiCapability capability) {
    switch (capability) {
        case AiCapability::Llm:
            return "llm";
        case AiCapability::Image:
            return "image";
        default:
            return "";
    }
}

std::string AiCredentialRules::capabilityLabel(AiCapability capability) {
    switch (capability) {
        case AiCapability::Llm:
            return "文本理解";
        case AiCapability::Image:
            return "图像生成";
        default:
            return "";
    }
}

const std::vector<AiProviderPreset> &AiCredentialRules::providers(AiCapability capability) {
    switch (capability) {
        case AiCapability::Llm:
            return kLlmProviders;
        case AiCapability::Image:
            return kImageProviders;
        default:
            return kEmptyProviders;
    }
}

const AiProviderPreset *AiCredentialRules::findProvider(AiCapability capability,
                                                       const std::string &providerId) {
    for (const auto &preset : providers(capability)) {
        if (preset.id == providerId) {
            return &preset;
        }
    }
    return nullptr;
}

json AiCredentialRules::catalogJson() {
    auto toJson = [](const std::vector<AiProviderPreset> &presets) {
        json items = json::array();
        for (const auto &preset : presets) {
            json models = json::array();
            for (const auto &model : preset.models) {
                models.push_back({{"id", model.id}, {"label", model.label}});
            }
            items.push_back({
                {"id", preset.id},
                {"label", preset.label},
                {"description", preset.description},
                {"api_base", preset.apiBase},
                {"key_url", preset.keyUrl},
                {"custom_base", preset.customBase},
                {"models", models},
            });
        }
        return items;
    };

    json catalog = json::object();
    catalog["llm"] = {
        {"label", capabilityLabel(AiCapability::Llm)},
        {"description", "把故事解析为角色、场景、动作等结构化数据"},
        {"providers", toJson(kLlmProviders)},
    };
    catalog["image"] = {
        {"label", capabilityLabel(AiCapability::Image)},
        {"description", "根据故事生成绘本插画"},
        {"providers", toJson(kImageProviders)},
    };
    return catalog;
}

std::string AiCredentialRules::defaultApiBase(AiCapability capability,
                                             const std::string &providerId) {
    const auto *preset = findProvider(capability, providerId);
    return preset == nullptr ? "" : preset->apiBase;
}

std::string AiCredentialRules::defaultModel(AiCapability capability,
                                           const std::string &providerId) {
    const auto *preset = findProvider(capability, providerId);
    if (preset == nullptr || preset->models.empty()) {
        return "";
    }
    return preset->models.front().id;
}

std::string AiCredentialRules::providerLabel(AiCapability capability,
                                             const std::string &providerId) {
    const auto *preset = findProvider(capability, providerId);
    return preset == nullptr ? providerId : preset->label;
}

std::string AiCredentialRules::keyUrl(AiCapability capability, const std::string &providerId) {
    const auto *preset = findProvider(capability, providerId);
    return preset == nullptr ? "" : preset->keyUrl;
}

std::string AiCredentialRules::normalizeApiBase(const std::string &apiBase) {
    std::string value = trim(apiBase);
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

bool AiCredentialRules::validateApiKey(const std::string &apiKey, std::string &error) {
    const std::string value = trim(apiKey);
    if (value.size() < 8 || value.size() > 200) {
        error = "API Key 长度需在 8–200 个字符之间";
        return false;
    }
    if (value != apiKey) {
        error = "API Key 不能包含首尾空格";
        return false;
    }
    if (!isAsciiPrintable(value)) {
        error = "API Key 含非法字符（不能有空格或控制字符）";
        return false;
    }
    return true;
}

bool AiCredentialRules::validateApiBase(const std::string &apiBase, std::string &error) {
    const std::string value = normalizeApiBase(apiBase);
    if (value.size() > 500) {
        error = "Base URL 过长（最多 500 字符）";
        return false;
    }
    if (value.rfind("https://", 0) != 0) {
        error = "Base URL 必须以 https:// 开头";
        return false;
    }
    if (!isAsciiPrintable(value)) {
        error = "Base URL 含非法字符（不能有空格或控制字符）";
        return false;
    }

    const std::string rest = value.substr(8);  // 去掉 https://
    const std::size_t authorityEnd = rest.find_first_of("/?#");
    const std::string authority = rest.substr(0, authorityEnd);
    if (authority.empty()) {
        error = "Base URL 缺少域名";
        return false;
    }
    if (authority.find('@') != std::string::npos) {
        error = "Base URL 不允许包含用户名或密码";
        return false;
    }
    if (rest.find('?') != std::string::npos || rest.find('#') != std::string::npos) {
        error = "Base URL 不允许包含查询参数或锚点";
        return false;
    }

    std::string host;
    std::string port;
    if (authority.front() == '[') {
        // 直接拒绝 IPv6 字面量：私网判断复杂且真实厂商几乎不用 IP 访问
        error = "Base URL 不支持 IP 地址，请填写域名";
        return false;
    }

    const std::size_t colon = authority.rfind(':');
    if (colon != std::string::npos && authority.find(':') == colon) {
        host = authority.substr(0, colon);
        port = authority.substr(colon + 1);
    } else {
        host = authority;
    }

    bool portOk = true;
    parsePort(port, portOk);
    if (!portOk) {
        error = "Base URL 仅支持 443 端口（https 默认端口）";
        return false;
    }

    std::transform(host.begin(), host.end(), host.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (!isValidHost(host)) {
        error = "Base URL 域名非法（不允许 localhost、单段主机名或 IP 地址）";
        return false;
    }
    if (isIpv4Literal(host)) {
        error = "Base URL 不允许使用 IP 地址";
        return false;
    }
    return true;
}

bool AiCredentialRules::validateModel(const std::string &model, std::string &error) {
    const std::string value = trim(model);
    if (value.empty()) {
        error = "请选择或填写模型名称";
        return false;
    }
    if (value.size() > 120) {
        error = "模型名称过长（最多 120 字符）";
        return false;
    }
    if (!isAsciiPrintable(value)) {
        error = "模型名称含非法字符";
        return false;
    }
    return true;
}

bool AiCredentialRules::validateConfig(AiCapability capability,
                                       const std::string &providerId,
                                       const std::string &apiKey,
                                       const std::string &apiBase,
                                       const std::string &model,
                                       std::string &error) {
    const auto *preset = findProvider(capability, providerId);
    if (preset == nullptr) {
        error = "不支持的厂商：" + providerId;
        return false;
    }
    if (!validateApiKey(apiKey, error)) {
        return false;
    }

    const std::string base = normalizeApiBase(apiBase);
    if (base.empty()) {
        if (preset->apiBase.empty()) {
            error = "自定义厂商必须填写 Base URL";
            return false;
        }
    } else {
        if (!validateApiBase(base, error)) {
            return false;
        }
        if (!preset->customBase && normalizeApiBase(preset->apiBase) != base) {
            error = "该厂商仅支持预设地址，如需更换请选择「自定义」";
            return false;
        }
    }

    const std::string modelName = trim(model);
    if (modelName.empty()) {
        if (preset->models.empty()) {
            error = "自定义厂商必须填写模型名称";
            return false;
        }
    } else if (!validateModel(modelName, error)) {
        return false;
    }

    if (capability == AiCapability::Image && preset->customBase) {
        error = "图像生成不支持自定义厂商";
        return false;
    }
    return true;
}

bool AiCredentialRules::shouldExemptQuota(bool llmConfigured, bool imageConfigured) {
    return llmConfigured && imageConfigured;
}
