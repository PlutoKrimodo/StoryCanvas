#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/BookPage.h"

#include "../models/GeneratedImage.h"

/** 页容器数据访问层：全部使用参数化 SQL。 */
class BookPageRepository {
public:
    /**
     * 把一组图片按顺序写入绘本的连续页（从当前最大页码之后开始）。
     * 单条 SQL 完成，避免只写入一部分页面。
     */
    static std::optional<std::vector<BookPage>> insertBatch(
        const std::string &bookId,
        const std::vector<GeneratedImage> &images);

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
