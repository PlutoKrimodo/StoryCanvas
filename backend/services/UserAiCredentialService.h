#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "ServiceResult.h"

using json = nlohmann::json;

/** 解密后的单条凭据（仅存在于内存，禁止序列化到日志/任务参数）。 */
struct AiCredentialSecret {
    std::string capability;
    std::string provider;
    std::string apiKey;
    std::string apiBase;
    std::string model;

    json toRequestJson() const;
};

/** 生成任务按快照解析出的凭据集合。 */
struct ResolvedAiCredentials {
    bool ok = true;
    std::string error;
    bool llmConfigured = false;
    bool imageConfigured = false;
    std::optional<AiCredentialSecret> llm;
    std::optional<AiCredentialSecret> image;
};

/**
 * BYOK 业务层。
 *
 * 职责：
 *   - 厂商默认值补齐、字段校验、加密落库、脱敏回显；
 *   - 生成任务创建时判断是否「双项配齐」（额度豁免）；
 *   - 任务执行时按快照解密，并明确「配置缺失/失效」的失败语义（绝不回退系统 Key）。
 */
class UserAiCredentialService {
public:
    /** GET /users/me/ai-credentials */
    static ServiceResult overview(const std::string &userId);

    /** GET /users/me/ai-credentials/providers */
    static ServiceResult providers();

    /** PUT /users/me/ai-credentials/{capability}；api_key 为空表示沿用旧 Key。 */
    static ServiceResult save(const std::string &userId,
                              const std::string &capability,
                              const json &body);

    /** DELETE /users/me/ai-credentials/{capability} */
    static ServiceResult remove(const std::string &userId, const std::string &capability);

    /** POST /users/me/ai-credentials/test */
    static ServiceResult test(const std::string &userId, const json &body);

    /**
     * 查询用户两项能力是否都已配置（用于额度豁免判断）。
     * 返回 false 表示数据库查询失败，调用方不得据此豁免额度。
     */
    static bool loadConfigured(const std::string &userId,
                               bool &llmConfigured,
                               bool &imageConfigured);

    /**
     * 按任务快照解析凭据。
     *
     * @param needLlm   任务创建时 byok_llm=true
     * @param needImage 任务创建时 byok_image=true
     *
     * 若 need* 为 true 但凭据缺失 / 解密失败，返回 ok=false 并给出可执行文案，
     * 由调用方把任务置为失败（不回退系统 Key）。
     */
    static ResolvedAiCredentials resolveForTask(const std::string &userId,
                                                bool needLlm,
                                                bool needImage);

private:
    struct PreparedCredential {
        std::string capability;
        std::string provider;
        std::string apiKey;
        std::string apiBase;
        std::string model;
    };

    /** 解析请求体、补默认值并校验；失败时把错误写入 result 并返回 false。 */
    static bool prepare(const std::string &userId,
                        const std::string &capabilityText,
                        const json &body,
                        PreparedCredential &out,
                        ServiceResult &result);
};
