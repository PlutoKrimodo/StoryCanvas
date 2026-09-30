#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/GenerationTask.h"

/** 生成任务数据访问层：全部使用参数化 SQL。 */
class GenerationTaskRepository {
public:
    /** 插入 pending 任务，成功时回填 id / created_at / updated_at。 */
    static bool create(GenerationTask &task);

    /** 回写任务结果（status / prompt / parsed_data / result_url / error / model / completed_at）。 */
    static bool updateResult(GenerationTask &task);

    /** 仅更新状态（用于 processing / cancelled）。 */
    static bool markStatus(const std::string &id, const std::string &status);

    /** 按 ID 查询（含最近一张生成图的派生字段）。 */
    static std::optional<GenerationTask> findById(const std::string &id);

    /** 按用户分页查询（bookId 为空表示不筛选），按创建时间倒序。 */
    static std::vector<GenerationTask> findByUserId(const std::string &userId,
                                                    const std::string &bookId,
                                                    int limit,
                                                    int offset);

    /** 统计用户任务数量（bookId 为空表示不筛选）。 */
    static int countByUserId(const std::string &userId, const std::string &bookId);

    /** 统计某状态的任务数（用于并发控制）。 */
    static int countByStatus(const std::string &status);

    /** 把长时间处于 pending/processing 的任务标记为 failed（启动自愈）。 */
    static int failStaleTasks();
};
