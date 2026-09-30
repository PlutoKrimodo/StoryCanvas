#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/GeneratedImage.h"

/** 生成图数据访问层：全部使用参数化 SQL。 */
class GeneratedImageRepository {
public:
    /** 插入生成图记录，成功时回填 id / created_at。 */
    static bool create(GeneratedImage &image);

    /** 按 ID 查询。 */
    static std::optional<GeneratedImage> findById(const std::string &id);

    /** 按 ID + 用户查询（归属校验用，避免越权探测资源是否存在）。 */
    static std::optional<GeneratedImage> findByIdAndUser(const std::string &id,
                                                         const std::string &userId);

    /** 按用户分页查询（bookId 为空表示不筛选）。 */
    static std::vector<GeneratedImage> findByUserId(const std::string &userId,
                                                    const std::string &bookId,
                                                    int limit,
                                                    int offset);

    /** 统计用户生成图数量（bookId 为空表示不筛选）。 */
    static int countByUserId(const std::string &userId, const std::string &bookId);

    /** 按 ID 删除。 */
    static bool deleteById(const std::string &id);
};
