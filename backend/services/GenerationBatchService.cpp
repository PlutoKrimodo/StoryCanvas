#include "GenerationBatchService.h"

#include <random>
#include <spdlog/spdlog.h>

#include "../repositories/BookRepository.h"
#include "../repositories/ConversationMessageRepository.h"
#include "../repositories/ConversationRepository.h"
#include "../repositories/GenerationBatchRepository.h"
#include "../repositories/GenerationTaskRepository.h"
#include "AiServiceClient.h"
#include "ConversationService.h"
#include "GenerationService.h"
#include "UserAiCredentialService.h"

namespace {

ServiceResult fail(int code, const std::string &message) {
    ServiceResult result;
    result.code = code;
    result.message = message;
    return result;
}

ServiceResult ok(const std::string &message, const json &data, int code = 200) {
    ServiceResult result;
    result.success = true;
    result.code = code;
    result.message = message;
    result.data = data;
    return result;
}

json panelsToJson(const AiStoryboardResult &source) {
    json panels = json::array();
    for (size_t i = 0; i < source.panels.size(); ++i) {
        panels.push_back({
            {"index", static_cast<int>(i)},
            {"text", source.panels[i].text},
        });
    }
    return panels;
}

std::string firstLine(const std::string &text) {
    const auto pos = text.find_first_of("\r\n");
    std::string value = pos == std::string::npos ? text : text.substr(0, pos);
    if (value.size() > 24) {
        value.resize(24);
    }
    return value.empty() ? "新的绘本故事" : value;
}

int randomSeed() {
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<int> distribution(1, 2147483646);
    return distribution(generator);
}

}  // namespace

ServiceResult GenerationBatchService::createPlan(const std::string &userId,
                                                 bool isGuest,
                                                 const CreatePlanRequest &req) {
    // 先读取 BYOK 状态：双项都由用户自带 Key 承担时，张数上限可放宽到 12
    bool byokLlm = false;
    bool byokImage = false;
    if (!isGuest) {
        if (!UserAiCredentialService::loadConfigured(userId, byokLlm, byokImage)) {
            return fail(500, "读取 API Key 配置失败，请稍后重试");
        }
    }

    const int imageCount = isGuest ? 1 : req.imageCount;
    // 双项自定义模型时花费全部由用户承担，允许自定义张数；其余情况仍限制 1–6，
    // 避免系统 Key 被批量消耗。
    const int maxCount = (byokLlm && byokImage) ? 12 : 6;
    if (imageCount < 1 || imageCount > maxCount) {
        return fail(422, maxCount == 12 ? "生成张数需在 1–12 之间"
                                        : "生成张数需在 1–6 之间");
    }
    if (isGuest) {
        // 游客的分镜本身也消耗系统 LLM：先检查 1 张额度，避免只出分镜不出图的白嫖路径
        ServiceResult quota;
        if (!GenerationService::checkImageQuota(userId, true, 1, false, false, quota)) {
            return quota;
        }
    }

    json llmCredential = json::object();
    if (byokLlm) {
        auto resolved = UserAiCredentialService::resolveForTask(userId, true, false);
        if (!resolved.ok) {
            return fail(409, resolved.error);
        }
        if (resolved.llm.has_value()) {
            llmCredential = resolved.llm->toRequestJson();
        }
    }

    AiStoryboardResult storyboard;
    std::string error;
    if (!AiServiceClient::generateStoryboard(
            req.text, imageCount, req.style, llmCredential, storyboard, error)) {
        return fail(502, error);
    }
    if (static_cast<int>(storyboard.panels.size()) != imageCount) {
        return fail(502, "分镜数量与请求不一致，请重试");
    }

    const std::string title = storyboard.title.empty() ? firstLine(req.text) : storyboard.title;
    const json panels = panelsToJson(storyboard);

    Conversation conversation;
    ConversationMessage userMessage;
    ConversationMessage storyboardMessage;
    if (!isGuest) {
        ServiceResult ensureResult;
        if (!ConversationService::ensure(userId, req.conversationId, title, req.style, req.size,
                                         conversation, ensureResult)) {
            return ensureResult;
        }

        json emptyPayload = json::object();
        if (!ConversationService::append(conversation.id, "user", "text", req.text,
                                         emptyPayload, userMessage)) {
            return fail(500, "保存会话消息失败");
        }

        json storyboardPayload = {
            {"panels", panels},
            {"consistency", storyboard.consistency},
            {"consistency_prompt", storyboard.consistencyPrompt},
            {"negative_prompt", storyboard.negativePrompt},
            {"status", "draft"},
            {"images", json::array()},
        };
        if (!ConversationService::append(conversation.id, "assistant", "storyboard",
                                         title, storyboardPayload, storyboardMessage)) {
            return fail(500, "保存分镜消息失败");
        }
    }

    GenerationBatch batch;
    batch.userId = userId;
    if (!isGuest) {
        batch.conversationId = conversation.id;
        batch.messageId = storyboardMessage.id;
    }
    batch.status = "draft";
    batch.style = req.style;
    batch.size = req.size;
    batch.panelCount = imageCount;
    batch.panels = panels;
    batch.consistencyPrompt = storyboard.consistencyPrompt;
    batch.negativePrompt = storyboard.negativePrompt;
    if (!storyboard.parsedData.is_null()) {
        batch.parsedData = storyboard.parsedData;
    }
    batch.byokLlm = byokLlm;
    batch.byokImage = byokImage;

    if (!GenerationBatchRepository::create(batch)) {
        return fail(500, "创建分镜草稿失败");
    }

    if (!isGuest) {
        storyboardMessage.payload["batch_id"] = batch.id;
        ConversationService::updateMessagePayload(storyboardMessage.id,
                                                  storyboardMessage.payload);
        ConversationRepository::touch(conversation.id);
    }

    json data = {
        {"plan_id", batch.id},
        {"batch_id", batch.id},
        {"conversation_id", isGuest ? json(nullptr) : json(conversation.id)},
        {"message_id", isGuest ? json(nullptr) : json(storyboardMessage.id)},
        {"title", title},
        {"style", req.style},
        {"size", req.size},
        {"panel_count", imageCount},
        {"panels", panels},
        {"consistency", storyboard.consistency},
        {"consistency_prompt", storyboard.consistencyPrompt},
        {"negative_prompt", storyboard.negativePrompt},
        {"status", "draft"},
        {"quota_exempt", batch.byokLlm && batch.byokImage},
    };
    return ok("分镜已生成", data, 201);
}

ServiceResult GenerationBatchService::updatePlan(const std::string &userId,
                                                 const std::string &batchId,
                                                 const UpdatePlanRequest &req) {
    auto batch = GenerationBatchRepository::findById(batchId);
    if (!batch.has_value()) {
        return fail(404, "分镜不存在");
    }
    if (batch->userId != userId) {
        return fail(403, "无权操作该分镜");
    }
    if (batch->status != "draft") {
        return fail(409, "分镜已开始生成，不能再编辑");
    }
    if (static_cast<int>(req.panels.size()) != batch->panelCount) {
        return fail(422, "分镜数量必须与确认的张数一致");
    }

    batch->panels = req.toJson();
    if (!GenerationBatchRepository::updateDraft(*batch)) {
        return fail(500, "保存分镜失败");
    }

    if (batch->messageId.has_value()) {
        auto message = ConversationMessageRepository::findById(*batch->messageId);
        if (message.has_value()) {
            message->payload["panels"] = batch->panels;
            ConversationMessageRepository::updatePayload(message->id, message->payload);
        }
    }

    return ok("分镜已保存", {
        {"batch_id", batch->id},
        {"panels", batch->panels},
        {"status", batch->status},
    });
}

ServiceResult GenerationBatchService::startBatch(const std::string &userId,
                                                 bool isGuest,
                                                 const std::string &batchId,
                                                 const StartBatchRequest &req) {
    auto batch = GenerationBatchRepository::findById(batchId);
    if (!batch.has_value()) {
        return fail(404, "分镜不存在");
    }
    if (batch->userId != userId) {
        return fail(403, "无权操作该分镜");
    }
    if (batch->status != "draft") {
        return fail(409, "该分镜已开始生成");
    }
    if (GenerationBatchRepository::countActiveByUser(userId) >= 1) {
        return fail(429, "已有进行中的生成批次，请等待完成后再试");
    }
    if (req.seed > 2147483647) {
        return fail(422, "随机种子超出范围");
    }

    ServiceResult quota;
    if (!GenerationService::checkImageQuota(userId, isGuest, batch->panelCount,
                                            batch->byokLlm, batch->byokImage, quota)) {
        return quota;
    }

    if (!req.bookId.empty()) {
        auto book = BookRepository::findById(req.bookId);
        if (!book.has_value()) {
            return fail(404, "绘本不存在");
        }
        if (book->userId != userId) {
            return fail(403, "无权使用该绘本");
        }
    }

    if (!batch->panels.is_array() ||
        static_cast<int>(batch->panels.size()) < batch->panelCount) {
        return fail(500, "分镜数据不完整，请重新拆分");
    }
    if (!req.style.empty()) batch->style = req.style;
    if (!req.size.empty()) batch->size = req.size;
    if (!GenerationBatchRepository::updateDraft(*batch)) {
        return fail(500, "保存确认参数失败");
    }

    const int seed = req.seed >= 0 ? req.seed : randomSeed();
    if (!GenerationBatchRepository::markStarted(batchId, seed)) {
        return fail(409, "分镜状态已变化，请刷新后重试");
    }

    std::vector<GenerationTask> tasks;
    for (int index = 0; index < batch->panelCount; ++index) {
        const auto &panel = batch->panels.at(static_cast<size_t>(index));
        const std::string panelText = panel.value("text", std::string());

        GenerationTask task;
        task.userId = userId;
        if (!req.bookId.empty()) task.bookId = req.bookId;
        task.originalText = panelText;
        task.prompt = "";
        task.byokLlm = batch->byokLlm;
        task.byokImage = batch->byokImage;
        task.isGuest = isGuest;
        task.batchId = batch->id;
        task.panelIndex = index;
        task.panelText = panelText;
        task.parameters = {
            {"style", batch->style},
            {"size", batch->size.value_or("1024x1024")},
            {"seed", seed},
        };

        if (!GenerationTaskRepository::create(task)) {
            GenerationBatchRepository::cancelPendingTasks(batchId);
            GenerationBatchRepository::updateStatus(batchId, "failed");
            return fail(500, "创建分镜任务失败");
        }
        tasks.push_back(task);
    }

    for (const auto &task : tasks) {
        if (!GenerationService::enqueueTask(task.id)) {
            GenerationTaskRepository::markStatus(task.id, "failed");
        }
    }
    GenerationBatchRepository::refreshStatus(batchId);

    auto updated = GenerationBatchRepository::findById(batchId);
    if (!updated.has_value()) {
        return fail(500, "读取批次状态失败");
    }
    return ok("已开始生成", buildView(*updated), 201);
}

ServiceResult GenerationBatchService::getBatch(const std::string &userId,
                                               const std::string &batchId) {
    auto batch = GenerationBatchRepository::findById(batchId);
    if (!batch.has_value()) {
        return fail(404, "批次不存在");
    }
    if (batch->userId != userId) {
        return fail(403, "无权访问该批次");
    }
    return ok("success", buildView(*batch));
}

ServiceResult GenerationBatchService::cancelBatch(const std::string &userId,
                                                  const std::string &batchId) {
    auto batch = GenerationBatchRepository::findById(batchId);
    if (!batch.has_value()) {
        return fail(404, "批次不存在");
    }
    if (batch->userId != userId) {
        return fail(403, "无权操作该批次");
    }
    if (batch->status == "draft") {
        GenerationBatchRepository::updateStatus(batchId, "cancelled");
    } else if (batch->status == "pending" || batch->status == "processing") {
        GenerationBatchRepository::cancelPendingTasks(batchId);
        GenerationBatchRepository::refreshStatus(batchId);
    } else {
        return fail(409, "批次已结束，无法取消");
    }

    auto updated = GenerationBatchRepository::findById(batchId);
    return updated.has_value() ? ok("批次已取消", buildView(*updated))
                               : fail(500, "读取批次状态失败");
}

ServiceResult GenerationBatchService::regeneratePanel(const std::string &userId,
                                                      bool isGuest,
                                                      const std::string &batchId,
                                                      int panelIndex) {
    auto batch = GenerationBatchRepository::findById(batchId);
    if (!batch.has_value()) {
        return fail(404, "批次不存在");
    }
    if (batch->userId != userId) {
        return fail(403, "无权操作该批次");
    }
    if (batch->status == "draft") {
        return fail(409, "请先开始生成，再重画单格");
    }

    auto tasks = GenerationTaskRepository::findByBatchId(batchId);
    if (!tasks.has_value()) {
        return fail(500, "查询分镜任务失败");
    }
    const GenerationTask *target = nullptr;
    for (const auto &task : *tasks) {
        if (task.panelIndex.has_value() && *task.panelIndex == panelIndex) {
            target = &task;
            break;
        }
    }
    if (target == nullptr) {
        return fail(404, "该格任务不存在");
    }
    if (target->status == "pending" || target->status == "processing") {
        return fail(409, "该格仍在生成中，请稍后再试");
    }

    ServiceResult quota;
    if (!GenerationService::checkImageQuota(userId, isGuest, 1,
                                            batch->byokLlm, batch->byokImage, quota)) {
        return quota;
    }

    if (!GenerationTaskRepository::resetForRetry(target->id,
                                                 target->panelText.value_or(""))) {
        return fail(500, "重置分镜任务失败");
    }
    if (!GenerationService::enqueueTask(target->id)) {
        GenerationTaskRepository::markStatus(target->id, "failed");
        return fail(500, "生成服务尚未就绪，请稍后重试");
    }
    GenerationBatchRepository::refreshStatus(batchId);

    auto updated = GenerationBatchRepository::findById(batchId);
    return updated.has_value() ? ok("已重新提交该格", buildView(*updated), 201)
                               : fail(500, "读取批次状态失败");
}

json GenerationBatchService::buildView(const GenerationBatch &batch) {
    json view = batch.toJson();
    auto tasks = GenerationTaskRepository::findByBatchId(batch.id);
    json items = json::array();
    if (tasks.has_value()) {
        for (const auto &task : *tasks) {
            const bool shown = task.status == "completed";
            items.push_back({
                {"task_id", task.id},
                {"panel_index", task.panelIndex.has_value() ? json(*task.panelIndex) : json(nullptr)},
                {"panel_text", task.panelText.has_value() ? json(*task.panelText) : json(nullptr)},
                {"status", task.status},
                {"image_id", shown && task.imageId.has_value() ? json(*task.imageId) : json(nullptr)},
                {"image_url", shown && task.resultUrl.has_value() ? json(*task.resultUrl) : json(nullptr)},
                {"prompt", task.prompt},
                {"model_used", task.modelUsed.has_value() ? json(*task.modelUsed) : json(nullptr)},
                {"error_message", task.errorMessage.has_value() ? json(*task.errorMessage) : json(nullptr)},
            });
        }
    }
    view["items"] = items;
    return view;
}
