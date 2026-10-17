#pragma once

#include <string>

#include "../dto/ConversationDto.h"
#include "../models/GenerationBatch.h"
#include "ServiceResult.h"

/**
 * 对话式多图生成：分镜草稿 → 额度预检 → N 个任务 → 批次聚合。
 *
 * 一个 batch 对应 N 个 generation_tasks；图像仍是一任务一图，
 * 以复用现有 worker、单格重画与失败隔离能力。
 */
class GenerationBatchService {
public:
    static ServiceResult createPlan(const std::string &userId,
                                    bool isGuest,
                                    const CreatePlanRequest &req);

    static ServiceResult updatePlan(const std::string &userId,
                                    const std::string &batchId,
                                    const UpdatePlanRequest &req);

    static ServiceResult startBatch(const std::string &userId,
                                    bool isGuest,
                                    const std::string &batchId,
                                    const StartBatchRequest &req);

    static ServiceResult getBatch(const std::string &userId, const std::string &batchId);

    static ServiceResult cancelBatch(const std::string &userId, const std::string &batchId);

    static ServiceResult regeneratePanel(const std::string &userId,
                                         bool isGuest,
                                         const std::string &batchId,
                                         int panelIndex);

private:
    static json buildView(const GenerationBatch &batch);
};
