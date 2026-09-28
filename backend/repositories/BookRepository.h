#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/Book.h"

// 绘本数据访问层：全部使用参数化 SQL
class BookRepository {
public:
    // 创建绘本，成功时回填 id / created_at / updated_at
    static bool create(Book &book);

    // 按 ID 查找
    static std::optional<Book> findById(const std::string &id);

    // 按用户分页查询（status 为空表示不筛选）
    static std::vector<Book> findByUserId(const std::string &userId,
                                          const std::string &status,
                                          int limit,
                                          int offset);

    // 统计用户绘本数量（status 为空表示不筛选）
    static int countByUserId(const std::string &userId, const std::string &status);

    // 更新绘本基础信息，成功时回填 updated_at
    static bool update(Book &book);

    // 按 ID 删除
    static bool deleteById(const std::string &id);
};
