#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/BookPage.h"

/** 页容器数据访问层：全部使用参数化 SQL。 */
class BookPageRepository {
public:
    /**
     * 保存页（存在则更新标题与图片绑定）。
     * 依赖 book_pages(book_id, page_number) 唯一约束实现 upsert。
     */
    static bool upsert(BookPage &page);

    /** 按绘本查询页（按页码升序，供 PDF 导出）。 */
    static std::vector<BookPage> findByBookId(const std::string &bookId);

    /** 按 ID 查询。 */
    static std::optional<BookPage> findById(const std::string &id);

    /** 统计绘本页数。 */
    static int countByBookId(const std::string &bookId);
};
