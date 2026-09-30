#include "AiServiceClient.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "../utils/ConfigManager.h"

namespace {

/**
 * 回调与工作线程之间的共享状态。
 *
 * 必须用 shared_ptr 持有：一旦等待超时，函数会提前返回，
 * 而回调仍可能在事件循环线程中稍后触发，用栈上对象会导致悬垂引用。
 */
struct SharedState {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    drogon::ReqResult result = drogon::ReqResult::NetworkFailure;
    drogon::HttpResponsePtr response;
};

bool parseResponse(const drogon::HttpResponsePtr &resp, AiImageResult &out, std::string &error) {
    if (resp == nullptr) {
        error = "AI 服务未返回响应";
        return false;
    }

    json body;
    try {
        body = json::parse(resp->getBody());
    } catch (const std::exception &e) {
        error = "AI 服务响应解析失败";
        spdlog::error("AI 服务响应解析失败: {}", e.what());
        return false;
    }

    if (resp->statusCode() != drogon::k200OK) {
        error = body.value("message", std::string("AI 服务返回错误"));
        return false;
    }

    try {
        out.prompt = body.value("prompt", std::string());
        out.negativePrompt = body.value("negative_prompt", std::string());
        out.modelUsed = body.value("model_used", std::string());
        if (body.contains("parsed_data") && !body["parsed_data"].is_null()) {
            out.parsedData = body["parsed_data"];
        }

        const auto &image = body.at("image");
        out.filePath = image.value("file_path", std::string());
        out.width = image.value("width", 0);
        out.height = image.value("height", 0);
        out.format = image.value("format", std::string());
        out.fileSize = image.value("file_size", static_cast<long long>(0));
    } catch (const std::exception &e) {
        error = "AI 服务响应缺少必要字段";
        spdlog::error("AI 服务响应字段缺失: {}", e.what());
        return false;
    }

    if (out.filePath.empty()) {
        error = "AI 服务未返回生成图路径";
        return false;
    }
    return true;
}

}  // namespace

bool AiServiceClient::generateImage(const std::string &text,
                                    const std::string &style,
                                    const std::string &userId,
                                    const std::string &bookId,
                                    const std::string &taskId,
                                    const json &parameters,
                                    AiImageResult &out,
                                    std::string &error) {
    json payload = {
        {"text", text},
        {"style", style},
        {"parameters", parameters.is_object() ? parameters : json::object()}
    };
    if (!userId.empty()) payload["user_id"] = userId;
    if (!bookId.empty()) payload["book_id"] = bookId;
    if (!taskId.empty()) payload["task_id"] = taskId;

    auto client = drogon::HttpClient::newHttpClient(ConfigManager::aiServiceUrl);
    // 注意：Drogon 的 newHttpJsonRequest 接收的是 jsoncpp 的 Json::Value，
    // 与本项目统一的 nlohmann::json 不是同一类型，故直接手工设置 JSON 字符串体
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/images/generate");
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    req->setBody(payload.dump());
    if (!ConfigManager::aiServiceApiKey.empty()) {
        req->addHeader("X-Internal-Api-Key", ConfigManager::aiServiceApiKey);
    }

    auto state = std::make_shared<SharedState>();
    client->sendRequest(req, [state](drogon::ReqResult result,
                                     const drogon::HttpResponsePtr &response) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->result = result;
        state->response = response;
        state->done = true;
        state->cv.notify_all();
    });

    {
        std::unique_lock<std::mutex> lock(state->mutex);
        const bool finished = state->cv.wait_for(
            lock, std::chrono::seconds(ConfigManager::aiServiceTimeout),
            [&state] { return state->done; });
        if (!finished) {
            error = "AI 服务调用超时（" + std::to_string(ConfigManager::aiServiceTimeout) + "s）";
            spdlog::error("调用 AI 服务超时: url={}", ConfigManager::aiServiceUrl);
            return false;
        }
    }

    if (state->result != drogon::ReqResult::Ok) {
        error = "AI 服务不可达（请确认 AI Service 已启动）";
        spdlog::error("调用 AI 服务失败: result={}", static_cast<int>(state->result));
        return false;
    }

    return parseResponse(state->response, out, error);
}
