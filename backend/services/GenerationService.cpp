#include "GenerationService.h"

#include <spdlog/spdlog.h>

#include <utility>

#include "../models/GenerationTask.h"
#include "../models/GeneratedImage.h"
#include "../repositories/BookPageRepository.h"
#include "../repositories/BookRepository.h"
#include "../repositories/GeneratedImageRepository.h"
#include "../repositories/GenerationTaskRepository.h"
#include "../repositories/GenerationBatchRepository.h"
#include "../utils/AiCredentialRules.h"
#include "../utils/ConfigManager.h"
#include "AiServiceClient.h"
#include "UserAiCredentialService.h"

// ==================== 静态成员定义 ====================
std::queue<std::string> GenerationService::queue_;
std::mutex GenerationService::queueMutex_;
std::condition_variable GenerationService::queueCv_;
std::vector<std::thread> GenerationService::workers_;
bool GenerationService::running_ = false;

namespace {

ServiceResult notFound(const std::string &message) {
    ServiceResult result;
    result.code = 404;
    result.message = message;
    return result;
}

ServiceResult forbidden(const std::string &message) {
    ServiceResult result;
    result.code = 403;
    result.message = message;
    return result;
}

ServiceResult serverError(const std::string &message) {
    ServiceResult result;
    result.code = 500;
    result.message = message;
    return result;
}

ServiceResult conflict(const std::string &message) {
    ServiceResult result;
    result.code = 409;
    result.message = message;
    return result;
}

/**
 * 游客配额校验：统计该游客「进行中 + 已完成」的任务数，达到上限即拒绝（429）。
 * 非游客不限制。
 */
bool checkGuestQuota(const std::string &userId,
                     bool isGuest,
                     int imageCount,
                     ServiceResult &result) {
    if (!isGuest) {
        return true;
    }
    const int used =
        GenerationTaskRepository::countByUserAndStatus(userId, "pending") +
        GenerationTaskRepository::countByUserAndStatus(userId, "processing") +
        GenerationTaskRepository::countByUserAndStatus(userId, "completed");
    if (used + imageCount > ConfigManager::guestGenerationQuota) {
        result.code = 429;
        result.message = "游客免费额度已用完（共 " +
                         std::to_string(ConfigManager::guestGenerationQuota) +
                         " 张），登录后可继续生成";
        return false;
    }
    return true;
}

/**
 * 游客每日池：所有游客共享，默认 20 张/天；与登录用户池完全独立。
 */
bool checkGuestDailyQuota(int imageCount, ServiceResult &result) {
    if (GenerationTaskRepository::countCreatedTodayByScope(true) + imageCount <=
        ConfigManager::guestDailyGenerationCap) {
        return true;
    }
    result.code = 429;
    result.message = "今日游客生成额度已用完（共 " +
                     std::to_string(ConfigManager::guestDailyGenerationCap) +
                     " 张），请明天再来或登录后继续";
    return false;
}

/**
 * 登录用户每日池：所有登录用户共享，默认 50 张/天；与游客池完全独立。
 */
bool checkUserDailyQuota(int imageCount, ServiceResult &result) {
    if (GenerationTaskRepository::countCreatedTodayByScope(false) + imageCount <=
        ConfigManager::userDailyGenerationCap) {
        return true;
    }
    result.code = 429;
    result.message = "今日登录用户生成额度已用完（共 " +
                     std::to_string(ConfigManager::userDailyGenerationCap) +
                     " 张），请明天再来";
    return false;
}

/** BYOK 任务快照：创建任务时两项能力是否都由用户自带 Key 承担。 */
struct ByokState {
    bool llm = false;
    bool image = false;
};

/**
 * 读取用户 BYOK 配置。
 *
 * 游客是影子用户、无法配置凭据，直接按「未配置」处理；
 * 数据库查询失败时拒绝创建任务，避免「以为免额度、实际扣系统成本」。
 */
bool resolveByokState(const std::string &userId,
                      bool isGuest,
                      ByokState &state,
                      ServiceResult &result) {
    if (isGuest) {
        return true;
    }
    if (!UserAiCredentialService::loadConfigured(userId, state.llm, state.image)) {
        result = serverError("读取你的 API Key 配置失败，请稍后重试");
        return false;
    }
    return true;
}

/**
 * 校验绘本归属。
 * 返回 true 表示通过；否则把错误写入 result。
 */
bool ensureBookOwnership(const std::string &userId,
                         const std::string &bookId,
                         ServiceResult &result) {
    auto book = BookRepository::findById(bookId);
    if (!book.has_value()) {
        result = notFound("绘本不存在");
        return false;
    }
    if (book->userId != userId) {
        result = forbidden("无权在该绘本下生成");
        return false;
    }
    return true;
}

}  // namespace

// ==================== 生命周期 ====================
void GenerationService::start() {
    if (running_) {
        return;
    }
    running_ = true;

    // 进程重启后内存队列已丢失，先把遗留的 pending/processing 收敛为失败
    const int stale = GenerationTaskRepository::failStaleTasks();
    if (stale > 0) {
        spdlog::warn("已将 {} 个遗留生成任务标记为失败（服务重启）", stale);
    }

    for (int i = 0; i < kWorkerCount; ++i) {
        workers_.emplace_back(&GenerationService::workerLoop);
    }
    spdlog::info("生成任务工作线程已启动（{} 个）", kWorkerCount);
}

void GenerationService::stop() {
    if (!running_) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        running_ = false;
    }
    queueCv_.notify_all();
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    spdlog::info("生成任务工作线程已停止");
}

bool GenerationService::enqueue(const std::string &taskId) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (!running_) {
            return false;
        }
        queue_.push(taskId);
    }
    queueCv_.notify_one();
    return true;
}

void GenerationService::workerLoop() {
    while (true) {
        std::string taskId;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [] { return !queue_.empty() || !running_; });
            if (!running_ && queue_.empty()) {
                return;
            }
            if (!queue_.empty()) {
                taskId = queue_.front();
                queue_.pop();
            }
        }
        if (!taskId.empty()) {
            processTask(taskId);
        }
    }
}

void GenerationService::processTask(const std::string &taskId) {
    auto taskOpt = GenerationTaskRepository::findById(taskId);
    if (!taskOpt.has_value()) {
        spdlog::warn("任务不存在，跳过: {}", taskId);
        return;
    }
    GenerationTask task = *taskOpt;

    // 已被取消的任务不再执行
    if (task.status == "cancelled") {
        spdlog::info("任务已取消，跳过执行: {}", taskId);
        return;
    }

    GenerationTaskRepository::markStatus(taskId, "processing");
    if (task.batchId.has_value()) {
        GenerationBatchRepository::refreshStatus(*task.batchId);
    }
    spdlog::info("开始处理生成任务: {}", taskId);

    AiImageResult aiResult;
    std::string error;
    const std::string bookId = task.bookId.value_or("");
    std::string style = task.parameters.is_object()
                            ? task.parameters.value("style", std::string("cartoon"))
                            : std::string("cartoon");

    // 批次上下文：N 个任务共享 parsed_data / consistency / negative / seed
    json parsedData = json(nullptr);
    std::string consistencyPrompt;
    std::string negativePromptExtra;
    if (task.batchId.has_value()) {
        auto batch = GenerationBatchRepository::findById(*task.batchId);
        if (!batch.has_value()) {
            task.status = "failed";
            task.errorMessage = "所属生成批次不存在，请重新开始创作";
            GenerationTaskRepository::updateResult(task);
            return;
        }
        style = batch->style;
        if (batch->parsedData.has_value()) {
            parsedData = *batch->parsedData;
        }
        consistencyPrompt = batch->consistencyPrompt.value_or("");
        negativePromptExtra = batch->negativePrompt.value_or("");
    }

    auto refreshBatch = [&task]() {
        if (task.batchId.has_value()) {
            GenerationBatchRepository::refreshStatus(*task.batchId);
        }
    };

    // BYOK：按任务快照解密凭据；缺失/失效时任务直接失败，绝不回退系统 Key
    json credentials = json::object();
    if (task.byokLlm || task.byokImage) {
        auto resolved = UserAiCredentialService::resolveForTask(
            task.userId, task.byokLlm, task.byokImage);
        if (!resolved.ok) {
            task.status = "failed";
            task.errorMessage = resolved.error;
            GenerationTaskRepository::updateResult(task);
            refreshBatch();
            spdlog::error("生成任务因 BYOK 凭据不可用而失败: {} | {}", taskId, resolved.error);
            return;
        }
        if (resolved.llm.has_value()) {
            credentials["llm_credential"] = resolved.llm->toRequestJson();
        }
        if (resolved.image.has_value()) {
            credentials["image_credential"] = resolved.image->toRequestJson();
        }
    }

    const bool ok = AiServiceClient::generateImage(
        task.originalText.value_or(task.prompt),
        style,
        task.userId,
        bookId,
        task.id,
        task.parameters,
        parsedData,
        consistencyPrompt,
        negativePromptExtra,
        credentials,
        aiResult,
        error);

    // 执行期间可能被取消，取消后不再回写结果
    auto latest = GenerationTaskRepository::findById(taskId);
    if (latest.has_value() && latest->status == "cancelled") {
        spdlog::info("任务在执行中被取消，丢弃结果: {}", taskId);
        return;
    }

    if (!ok) {
        if (task.byokLlm || task.byokImage) {
            error = "本次使用你配置的 API Key；失败原因：" + error +
                    "。请到个人中心检查 Key 后重试";
        }
        task.status = "failed";
        task.errorMessage = error;
        GenerationTaskRepository::updateResult(task);
        refreshBatch();
        spdlog::error("生成任务失败: {} | {}", taskId, error);
        return;
    }

    // 落生成图记录
    GeneratedImage image;
    image.taskId = task.id;
    image.userId = task.userId;
    image.filePath = aiResult.filePath;
    image.fileSize = aiResult.fileSize;
    image.width = aiResult.width;
    image.height = aiResult.height;
    image.format = aiResult.format;
    image.promptUsed = aiResult.prompt.empty()
                           ? task.originalText
                           : std::optional<std::string>(aiResult.prompt);
    image.parameters = task.parameters;

    if (!GeneratedImageRepository::create(image)) {
        task.status = "failed";
        task.errorMessage = "生成图记录保存失败";
        GenerationTaskRepository::updateResult(task);
        refreshBatch();
        spdlog::error("生成图落库失败: task={}", taskId);
        return;
    }

    task.status = "completed";
    task.prompt = aiResult.prompt;
    if (!aiResult.parsedData.is_null()) {
        task.parsedData = aiResult.parsedData;
    }
    task.resultUrl = image.imageUrl();
    task.modelUsed = aiResult.modelUsed;
    GenerationTaskRepository::updateResult(task);
    refreshBatch();
    spdlog::info("生成任务完成: {} | image={}", taskId, image.id);
}

// ==================== 业务接口 ====================
ServiceResult GenerationService::createTask(const std::string &userId,
                                            bool isGuest,
                                            const CreateGenerationRequest &req) {
    ServiceResult result;

    // 0. BYOK 快照：双项都由用户自带 Key 时豁免系统额度；
    //    只配一项（或未配置）仍照常占用全站 / 游客额度。
    ByokState byok;
    if (!resolveByokState(userId, isGuest, byok, result)) {
        return result;
    }
    if (!checkImageQuota(userId, isGuest, 1, byok.llm, byok.image, result)) {
        return result;
    }

    // 1. 归属校验（可选绑定绘本 / 页）
    if (!req.bookId.empty() && !ensureBookOwnership(userId, req.bookId, result)) {
        return result;
    }
    if (!req.pageId.empty()) {
        auto page = BookPageRepository::findById(req.pageId);
        if (!page.has_value()) {
            return notFound("页不存在");
        }
        if (!ensureBookOwnership(userId, page->bookId, result)) {
            return result;
        }
    }

    // 2. 落库
    GenerationTask task;
    task.userId = userId;
    if (!req.bookId.empty()) task.bookId = req.bookId;
    if (!req.pageId.empty()) task.pageId = req.pageId;
    task.originalText = req.text;
    task.prompt = "";
    task.byokLlm = byok.llm;
    task.byokImage = byok.image;
    task.isGuest = isGuest;
    task.parameters = req.parameters.is_object() ? req.parameters : json::object();
    task.parameters["style"] = req.style;  // 供工作线程回传给 AI 服务

    if (!GenerationTaskRepository::create(task)) {
        return serverError("创建生成任务失败");
    }

    // 3. 入队
    if (!enqueue(task.id)) {
        // 工作线程未启动（理论上不会发生）：标记失败，避免任务永远 pending
        GenerationTaskRepository::markStatus(task.id, "failed");
        return serverError("生成服务尚未就绪，请稍后重试");
    }

    result.success = true;
    result.code = 201;
    result.message = "任务创建成功";
    result.data = {
        {"task_id", task.id},
        {"status", "pending"},
        {"created_at", task.createdAt.value_or("")}
    };
    return result;
}

ServiceResult GenerationService::getTask(const std::string &userId, const std::string &taskId) {
    ServiceResult result;

    auto task = GenerationTaskRepository::findById(taskId);
    if (!task.has_value()) {
        return notFound("任务不存在");
    }
    if (task->userId != userId) {
        return forbidden("无权访问该任务");
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = task->toJson();
    return result;
}

ServiceResult GenerationService::listHistory(const std::string &userId,
                                             int page,
                                             int limit,
                                             const std::string &bookId) {
    ServiceResult result;

    if (page < 1) page = 1;
    if (limit < 1) limit = 20;
    if (limit > 100) limit = 100;
    const int offset = (page - 1) * limit;

    const auto tasks = GenerationTaskRepository::findByUserId(userId, bookId, limit, offset);
    if (!tasks.has_value()) {
        // 查询失败与「确实没有历史」必须区分：前者回 500，前端会弹出明确错误提示
        result.code = 500;
        result.message = "查询生成历史失败，请稍后重试";
        return result;
    }
    const int total = GenerationTaskRepository::countByUserId(userId, bookId);

    json items = json::array();
    for (const auto &task : *tasks) {
        items.push_back(task.toJson());
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = {
        {"items", items},
        {"total", total},
        {"page", page},
        {"limit", limit}
    };
    return result;
}

ServiceResult GenerationService::cancelTask(const std::string &userId,
                                            const std::string &taskId) {
    ServiceResult result;

    auto task = GenerationTaskRepository::findById(taskId);
    if (!task.has_value()) {
        return notFound("任务不存在");
    }
    if (task->userId != userId) {
        return forbidden("无权操作该任务");
    }
    if (task->status == "completed" || task->status == "failed" ||
        task->status == "cancelled") {
        return conflict("任务已结束，无法取消");
    }

    if (!GenerationTaskRepository::markStatus(taskId, "cancelled")) {
        return serverError("取消任务失败");
    }

    result.success = true;
    result.code = 200;
    result.message = "任务已取消";
    return result;
}

ServiceResult GenerationService::regenerate(const std::string &userId,
                                            bool isGuest,
                                            const std::string &taskId,
                                            const RegenerateRequest &req) {
    ServiceResult result;

    // 重新生成按「当前」配置重新打快照；双项 BYOK 仍豁免系统额度
    ByokState byok;
    if (!resolveByokState(userId, isGuest, byok, result)) {
        return result;
    }
    if (!checkImageQuota(userId, isGuest, 1, byok.llm, byok.image, result)) {
        return result;
    }

    auto origin = GenerationTaskRepository::findById(taskId);
    if (!origin.has_value()) {
        return notFound("原任务不存在");
    }
    if (origin->userId != userId) {
        return forbidden("无权操作该任务");
    }

    // 复用原任务的文本 / 风格 / 归属，参数可覆盖
    GenerationTask task;
    task.userId = userId;
    task.bookId = origin->bookId;
    task.pageId = origin->pageId;
    task.originalText = origin->originalText;
    task.prompt = "";
    task.byokLlm = byok.llm;
    task.byokImage = byok.image;
    task.isGuest = isGuest;
    task.parameters = origin->parameters;
    if (req.parameters.is_object()) {
        for (auto it = req.parameters.begin(); it != req.parameters.end(); ++it) {
            task.parameters[it.key()] = it.value();
        }
    }

    if (!GenerationTaskRepository::create(task)) {
        return serverError("创建重新生成任务失败");
    }
    if (!enqueue(task.id)) {
        GenerationTaskRepository::markStatus(task.id, "failed");
        return serverError("生成服务尚未就绪，请稍后重试");
    }

    result.success = true;
    result.code = 201;
    result.message = "重新生成任务创建成功";
    result.data = {
        {"task_id", task.id},
        {"status", "pending"},
        {"created_at", task.createdAt.value_or("")}
    };
    return result;
}

bool GenerationService::checkImageQuota(const std::string &userId,
                                        bool isGuest,
                                        int count,
                                        bool byokLlm,
                                        bool byokImage,
                                        ServiceResult &result) {
    if (count < 1) {
        result.code = 422;
        result.message = "生成张数必须至少为 1";
        return false;
    }
    if (AiCredentialRules::shouldExemptQuota(byokLlm, byokImage)) {
        return true;
    }
    if (isGuest) {
        if (!checkGuestDailyQuota(count, result)) {
            return false;
        }
        return checkGuestQuota(userId, true, count, result);
    }
    return checkUserDailyQuota(count, result);
}

bool GenerationService::enqueueTask(const std::string &taskId) {
    return enqueue(taskId);
}
