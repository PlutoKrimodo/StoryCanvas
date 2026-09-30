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
                              AiImageResult &out,
                              std::string &error);
};
