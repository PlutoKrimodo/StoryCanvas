#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

/**
 * BYOK 厂商目录与校验规则（后端唯一来源）。
 *
 * 设计目标：
 *   - 前端只渲染这里下发的厂商 / 模型 / 申请链接，不在前端硬编码模型清单；
 *   - AI 服务只关心「OpenAI 兼容」或「豆包 / 万相」协议，不关心厂商展示信息；
 *   - 自定义 Base URL 仅对 LLM 开放，且必须在保存前通过 HTTPS / 非 IP 字面量校验。
 *
 * 本类不依赖 Drogon，可直接单测。
 */
enum class AiCapability {
    Llm,
    Image,
    Unknown,
};

struct AiModelOption {
    std::string id;
    std::string label;
};

struct AiProviderPreset {
    std::string id;
    std::string label;
    std::string description;
    std::string apiBase;
    std::string keyUrl;
    /// 允许用户填写自己的 Base URL（仅 LLM 自定义厂商为 true）
    bool customBase = false;
    std::vector<AiModelOption> models;
};

class AiCredentialRules {
public:
    static bool parseCapability(const std::string &text, AiCapability &capability);
    static std::string capabilityName(AiCapability capability);   // llm / image
    static std::string capabilityLabel(AiCapability capability);  // 文本理解 / 图像生成

    static const std::vector<AiProviderPreset> &providers(AiCapability capability);
    static const AiProviderPreset *findProvider(AiCapability capability,
                                                const std::string &providerId);

    /** 前端 `GET /users/me/ai-credentials/providers` 的响应体。 */
    static json catalogJson();

    static std::string defaultApiBase(AiCapability capability, const std::string &providerId);
    static std::string defaultModel(AiCapability capability, const std::string &providerId);
    static std::string providerLabel(AiCapability capability, const std::string &providerId);
    static std::string keyUrl(AiCapability capability, const std::string &providerId);

    /** 去掉首尾空白与尾部 '/'；不做协议补全。 */
    static std::string normalizeApiBase(const std::string &apiBase);

    static bool validateApiKey(const std::string &apiKey, std::string &error);
    static bool validateApiBase(const std::string &apiBase, std::string &error);
    static bool validateModel(const std::string &model, std::string &error);

    /**
     * 保存前整体校验。
     *
     * @param apiBase 允许为空（表示使用预设地址）；图像能力必须是预设地址，
     *                否则返回 false —— 图像两家协议不同，不支持任意 Base URL。
     * @param model   允许为空（表示使用预设默认模型）；自定义厂商必须填写。
     */
    static bool validateConfig(AiCapability capability,
                               const std::string &providerId,
                               const std::string &apiKey,
                               const std::string &apiBase,
                               const std::string &model,
                               std::string &error);

    /** 仅当 LLM 与图像都使用用户自带 Key 时，才豁免系统额度。 */
    static bool shouldExemptQuota(bool llmConfigured, bool imageConfigured);
};
