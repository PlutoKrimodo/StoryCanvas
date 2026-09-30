#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "ServiceResult.h"
#include "../dto/GenerationDto.h"

/**
 * 生成任务服务（Phase 5 核心）。
 *
 * 架构（对照 docs/08 第一阶段：内存队列）：
 *   创建任务 → 落库(pending) → 入内存队列 → 工作线程取任务
 *   → 标记 processing → 调 AI Service → 落生成图 → 回写 completed/failed
 *
 * 任务状态的**唯一权威来源是数据库**，内存队列只承担"待处理 id"的传递，
 * 因此进程重启后通过 failStaleTasks() 把遗留任务收敛为 failed 即可，不会丢状态。
 */
class GenerationService {
public:
    // ---- 生命周期（由 main 调用）----
    static void start();
    static void stop();

    // ---- 业务接口 ----
    static ServiceResult createTask(const std::string &userId, const CreateGenerationRequest &req);
    static ServiceResult getTask(const std::string &userId, const std::string &taskId);
    static ServiceResult listHistory(const std::string &userId,
                                     int page,
                                     int limit,
                                     const std::string &bookId);
    static ServiceResult cancelTask(const std::string &userId, const std::string &taskId);
    static ServiceResult regenerate(const std::string &userId,
                                    const std::string &taskId,
                                    const RegenerateRequest &req);

private:
    static bool enqueue(const std::string &taskId);
    static void workerLoop();
    static void processTask(const std::string &taskId);

    // ---- 队列与工作线程 ----
    static std::queue<std::string> queue_;
    static std::mutex queueMutex_;
    static std::condition_variable queueCv_;
    static std::vector<std::thread> workers_;
    static bool running_;

    // 并发工作线程数（MVP 设为 2，避免单点阻塞）
    static constexpr int kWorkerCount = 2;
};
