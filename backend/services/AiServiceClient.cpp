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
                                    const json &parsedData,
                                    const std::string &consistencyPrompt,
                                    const std::string &negativePromptExtra,
                                    const json &credentials,
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
    if (!parsedData.is_null()) {
        payload["parsed_data"] = parsedData;
    }
    if (!consistencyPrompt.empty()) {
        payload["consistency_prompt"] = consistencyPrompt;
    }
    if (!negativePromptExtra.empty()) {
        payload["negative_prompt_extra"] = negativePromptExtra;
    }
    // BYOK：只放本次请求所需的解密凭据，任务 parameters 中绝不携带 Key
    if (credentials.is_object()) {
        if (credentials.contains("llm_credential")) {
            payload["llm_credential"] = credentials["llm_credential"];
        }
        if (credentials.contains("image_credential")) {
            payload["image_credential"] = credentials["image_credential"];
        }
    }

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

bool AiServiceClient::testCredential(const std::string &capability,
                                     const std::string &provider,
                                     const std::string &apiKey,
                                     const std::string &apiBase,
                                     const std::string &model,
                                     AiCredentialTestResult &out,
                                     std::string &error) {
    json payload = {
        {"capability", capability},
        {"provider", provider},
        {"api_key", apiKey}
    };
    if (!apiBase.empty()) payload["api_base"] = apiBase;
    if (!model.empty()) payload["model"] = model;

    auto client = drogon::HttpClient::newHttpClient(ConfigManager::aiServiceUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/credentials/test");
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    req->setBody(payload.dump());
    if (!ConfigManager::aiServiceApiKey.empty()) {
        req->addHeader("X-Internal-Api-Key", ConfigManager::aiServiceApiKey);
    }

    const auto started = std::chrono::steady_clock::now();
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
        constexpr int kTestTimeoutSeconds = 10;
        const bool finished = state->cv.wait_for(
            lock, std::chrono::seconds(kTestTimeoutSeconds),
            [&state] { return state->done; });
        if (!finished) {
            error = "测试连接超时（10s），请检查网络或稍后重试";
            return false;
        }
    }

    if (state->result != drogon::ReqResult::Ok || state->response == nullptr) {
        error = "AI 服务不可达（请确认 AI Service 已启动）";
        spdlog::error("凭据测试调用失败: result={}", static_cast<int>(state->result));
        return false;
    }

    json body;
    try {
        body = json::parse(state->response->getBody());
    } catch (const std::exception &) {
        error = "AI 服务响应解析失败";
        return false;
    }

    if (state->response->statusCode() != drogon::k200OK) {
        // FastAPI 校验失败时 detail 可能是数组，必须判类型后再取，避免 nlohmann 抛异常
        std::string message = body.value("message", std::string());
        if (message.empty() && body.contains("detail") && body["detail"].is_string()) {
            message = body["detail"].get<std::string>();
        }
        error = message.empty() ? "凭据测试失败" : message;
        return false;
    }

    out.ok = body.value("ok", false);
    out.verified = body.value("verified", false);
    out.latencyMs = body.value("latency_ms", 0);
    out.message = body.value("message", std::string());
    if (out.message.empty()) {
        out.message = out.ok ? "连接成功" : "连接失败";
    }
    spdlog::info("凭据测试完成: capability={} provider={} ok={} latency={}ms",
                 capability, provider, out.ok,
                 static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - started)
                                      .count()));
    return true;
}

bool AiServiceClient::generateStoryboard(const std::string &text,
                                         int imageCount,
                                         const std::string &style,
                                         const json &llmCredential,
                                         AiStoryboardResult &out,
                                         std::string &error) {
    json payload = {
        {"text", text},
        {"image_count", imageCount},
        {"style", style}
    };
    if (llmCredential.is_object() && !llmCredential.empty()) {
        payload["llm_credential"] = llmCredential;
    }

    auto client = drogon::HttpClient::newHttpClient(ConfigManager::aiServiceUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/v1/analysis/storyboard");
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
        constexpr int kStoryboardTimeoutSeconds = 60;
        const bool finished = state->cv.wait_for(
            lock, std::chrono::seconds(kStoryboardTimeoutSeconds),
            [&state] { return state->done; });
        if (!finished) {
            error = "分镜生成超时（60s），请稍后重试";
            return false;
        }
    }

    if (state->result != drogon::ReqResult::Ok || state->response == nullptr) {
        error = "AI 服务不可达（请确认 AI Service 已启动）";
        spdlog::error("分镜调用失败: result={}", static_cast<int>(state->result));
        return false;
    }

    json body;
    try {
        body = json::parse(state->response->getBody());
    } catch (const std::exception &) {
        error = "AI 服务响应解析失败";
        return false;
    }

    if (state->response->statusCode() != drogon::k200OK) {
        std::string message = body.value("message", std::string());
        if (message.empty() && body.contains("detail") && body["detail"].is_string()) {
            message = body["detail"].get<std::string>();
        }
        error = message.empty() ? "分镜生成失败" : message;
        return false;
    }

    try {
        out.title = body.value("title", std::string());
        out.consistency = body.value("consistency", json::object());
        out.consistencyPrompt = body.value("consistency_prompt", std::string());
        out.negativePrompt = body.value("negative_prompt", std::string());
        if (body.contains("parsed_data") && !body["parsed_data"].is_null()) {
            out.parsedData = body["parsed_data"];
        }
        out.panels.clear();
        const auto &panels = body.at("panels");
        if (!panels.is_array()) {
            error = "分镜响应缺少 panels";
            return false;
        }
        for (const auto &item : panels) {
            AiStoryboardPanel panel;
            panel.index = item.value("index", static_cast<int>(out.panels.size()));
            panel.text = item.value("text", std::string());
            if (!panel.text.empty()) {
                out.panels.push_back(panel);
            }
        }
    } catch (const std::exception &) {
        error = "分镜响应字段缺失";
        return false;
    }

    if (out.panels.empty()) {
        error = "分镜结果为空";
        return false;
    }
    return true;
}
