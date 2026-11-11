#include "ConversationService.h"

#include <algorithm>
#include <map>
#include <spdlog/spdlog.h>

#include "../repositories/ConversationMessageRepository.h"
#include "../repositories/ConversationRepository.h"
#include "../repositories/GenerationBatchRepository.h"
#include "../repositories/GenerationTaskRepository.h"

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

json buildBatchItems(const std::string &batchId) {
    auto tasks = GenerationTaskRepository::findByBatchId(batchId);
    json items = json::array();
    if (!tasks.has_value()) {
        return items;
    }
    for (const auto &task : *tasks) {
        const bool shown = task.status == "completed";
        const auto errorCode = task.errorCode();
        items.push_back({
            {"task_id", task.id},
            {"panel_index", task.panelIndex.has_value() ? json(*task.panelIndex) : json(nullptr)},
            {"status", task.status},
            {"image_id", shown && task.imageId.has_value() ? json(*task.imageId) : json(nullptr)},
            {"image_url", shown && task.resultUrl.has_value() ? json(*task.resultUrl) : json(nullptr)},
            {"error_message", task.errorMessage.has_value() ? json(*task.errorMessage) : json(nullptr)},
            {"error_code", errorCode.has_value() ? json(*errorCode) : json(nullptr)},
        });
    }
    return items;
}

}  // namespace

ServiceResult ConversationService::create(const std::string &userId,
                                          const CreateConversationRequest &req) {
    Conversation conversation;
    conversation.userId = userId;
    if (!req.title.empty()) conversation.title = req.title;
    if (!req.style.empty()) conversation.style = req.style;
    if (!req.size.empty()) conversation.size = req.size;

    if (!ConversationRepository::create(conversation)) {
        return serverError("创建会话失败");
    }

    ServiceResult result;
    result.success = true;
    result.code = 201;
    result.message = "会话创建成功";
    result.data = conversation.toJson();
    return result;
}

ServiceResult ConversationService::list(const std::string &userId, int page, int limit) {
    const int safePage = std::max(1, page);
    const int safeLimit = std::min(std::max(1, limit), 50);

    auto conversations =
        ConversationRepository::findByUser(userId, safeLimit, (safePage - 1) * safeLimit);
    if (!conversations.has_value()) {
        return serverError("查询会话列表失败");
    }

    json items = json::array();
    for (const auto &conversation : *conversations) {
        items.push_back(conversation.toJson());
    }

    ServiceResult result;
    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = {
        {"items", items},
        {"total", ConversationRepository::countByUser(userId)},
        {"page", safePage},
        {"limit", safeLimit},
    };
    return result;
}

ServiceResult ConversationService::get(const std::string &userId,
                                       const std::string &conversationId) {
    auto conversation = ConversationRepository::findById(conversationId);
    if (!conversation.has_value()) {
        return notFound("会话不存在");
    }
    if (conversation->userId != userId) {
        return forbidden("无权访问该会话");
    }

    auto messages = ConversationMessageRepository::findByConversation(conversationId);
    if (!messages.has_value()) {
        return serverError("查询会话消息失败");
    }

    auto batches = GenerationBatchRepository::findByConversationId(conversationId);
    if (!batches.has_value()) {
        return serverError("查询会话批次失败");
    }
    std::map<std::string, GenerationBatch> batchByMessage;
    for (const auto &batch : *batches) {
        if (batch.messageId.has_value()) {
            batchByMessage[*batch.messageId] = batch;
        }
    }

    json items = json::array();
    for (auto message : *messages) {
        json payload = message.payload;
        std::string batchId;
        if (payload.contains("batch_id") && payload["batch_id"].is_string()) {
            batchId = payload["batch_id"].get<std::string>();
        }
        if (!batchId.empty() && batchByMessage.count(message.id) > 0) {
            const auto &batch = batchByMessage[message.id];
            payload["status"] = batch.status;
            payload["batch_status"] = batch.status;
            payload["seed"] = batch.seed.has_value() ? json(*batch.seed) : json(nullptr);
            payload["images"] = buildBatchItems(batch.id);
        }
        message.payload = payload;
        items.push_back(message.toJson());
    }

    ServiceResult result;
    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = {
        {"conversation", conversation->toJson()},
        {"messages", items},
    };
    return result;
}

ServiceResult ConversationService::remove(const std::string &userId,
                                          const std::string &conversationId) {
    auto conversation = ConversationRepository::findById(conversationId);
    if (!conversation.has_value()) {
        return notFound("会话不存在");
    }
    if (conversation->userId != userId) {
        return forbidden("无权删除该会话");
    }
    if (!ConversationRepository::remove(conversationId, userId)) {
        return serverError("删除会话失败");
    }

    ServiceResult result;
    result.success = true;
    result.code = 200;
    result.message = "会话已删除";
    return result;
}

bool ConversationService::ensure(const std::string &userId,
                                 const std::string &conversationId,
                                 const std::string &title,
                                 const std::string &style,
                                 const std::string &size,
                                 Conversation &out,
                                 ServiceResult &result) {
    if (!conversationId.empty()) {
        auto existing = ConversationRepository::findById(conversationId);
        if (!existing.has_value()) {
            result = notFound("会话不存在");
            return false;
        }
        if (existing->userId != userId) {
            result = forbidden("无权访问该会话");
            return false;
        }
        out = *existing;
        ConversationRepository::updateMeta(out.id, title, style, size);
        out.title = title.empty() ? out.title : std::optional<std::string>(title);
        out.style = style.empty() ? out.style : std::optional<std::string>(style);
        out.size = size.empty() ? out.size : std::optional<std::string>(size);
        return true;
    }

    out.userId = userId;
    if (!title.empty()) out.title = title;
    if (!style.empty()) out.style = style;
    if (!size.empty()) out.size = size;
    if (!ConversationRepository::create(out)) {
        result = serverError("创建会话失败");
        return false;
    }
    return true;
}

bool ConversationService::append(const std::string &conversationId,
                                 const std::string &role,
                                 const std::string &messageType,
                                 const std::string &content,
                                 const json &payload,
                                 ConversationMessage &out) {
    out.conversationId = conversationId;
    out.role = role;
    out.messageType = messageType;
    out.content = content;
    out.payload = payload.is_object() ? payload : json::object();
    return ConversationMessageRepository::create(out);
}

bool ConversationService::updateMessagePayload(const std::string &messageId,
                                               const json &payload) {
    return ConversationMessageRepository::updatePayload(messageId, payload);
}
