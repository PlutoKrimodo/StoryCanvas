#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/** AI 服务返回的生成图信息。 */
struct AiImageResult {
    std::string filePath;   // 相对存储根目录的路径
    int width = 0;
    int height = 0;
    std::string format;
    long long fileSize = 0;
    std::string prompt;
    std::string negativePrompt;
    json parsedData = json(nullptr);
    std::string modelUsed;
};

/** 凭据测试结果（AI 服务已把厂商错误归一为可读 message）。 */
struct AiCredentialTestResult {
    bool ok = false;
    bool verified = false;   // LLM 为真实探测；图像为格式校验，固定 false
    int latencyMs = 0;
    std::string message;
};

/**
 * 后端 → Python AI Service 的内部调用客户端。
 *
 * 使用 Drogon HttpClient（异步接口）封装为**阻塞调用**，
 * 供生成任务的工作线程使用；超时与错误统一转为 error 字符串，不抛异常。
 */
class AiServiceClient {
public:
    /**
     * 调用 `POST {AI_SERVICE_URL}/api/v1/images/generate`。
     * 成功返回 true 并填充 out；失败返回 false 并填充 error。
     */
    static bool generateImage(const std::string &text,
                              const std::string &style,
                              const std::string &userId,
                              const std::string &bookId,
                              const std::string &taskId,
                              const json &parameters,
                              const json &credentials,
                              AiImageResult &out,
                              std::string &error);

    /**
     * 调用 `POST {AI_SERVICE_URL}/api/v1/credentials/test`（10 秒超时）。
     *
     * 返回 false 仅表示「未能拿到 AI 服务响应」（网络 / 响应格式问题）；
     * 厂商拒绝 Key 属于业务结果，返回 true 且 out.ok=false。
     */
    static bool testCredential(const std::string &capability,
                               const std::string &provider,
                               const std::string &apiKey,
                               const std::string &apiBase,
                               const std::string &model,
                               AiCredentialTestResult &out,
                               std::string &error);
};
