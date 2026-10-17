#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../models/GenerationBatch.h"

/** 批次数据访问：分镜草稿、启动、状态聚合与取消。 */
class GenerationBatchRepository {
public:
    static bool create(GenerationBatch &batch);

    static std::optional<GenerationBatch> findById(const std::string &id);

    static std::optional<GenerationBatch> findByMessageId(const std::string &messageId);

    static std::optional<std::vector<GenerationBatch>> findByConversationId(
        const std::string &conversationId);

    /** 保存用户编辑后的分镜与风格；仅 draft 状态可改。 */
    static bool updateDraft(GenerationBatch &batch);

    /** draft → pending，并固化 seed。 */
    static bool markStarted(const std::string &id, int seed);

    static bool updateStatus(const std::string &id, const std::string &status);

    /** 按任务状态聚合批次状态（completed / partial / failed / cancelled / processing / pending）。 */
    static bool refreshStatus(const std::string &id);

    /** 取消批次中仍处于 pending 的任务；返回取消数量。 */
    static int cancelPendingTasks(const std::string &id);

    static int countActiveByUser(const std::string &userId);
};
