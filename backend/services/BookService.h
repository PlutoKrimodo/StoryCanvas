#pragma once

#include <string>
#include "ServiceResult.h"
#include "../dto/BookDto.h"

// 绘本业务逻辑层：所有单本操作均做资源归属校验
class BookService {
public:
    // 创建绘本（归属当前用户）
    static ServiceResult createBook(const std::string &userId, const CreateBookRequest &req);

    // 分页查询当前用户的绘本（status 为空表示不筛选）
    static ServiceResult listBooks(const std::string &userId, int page, int limit, const std::string &status);

    // 获取绘本详情（校验归属）
    static ServiceResult getBook(const std::string &userId, const std::string &bookId);

    // 更新绘本信息（校验归属）
    static ServiceResult updateBook(const std::string &userId, const std::string &bookId, const UpdateBookRequest &req);

    // 删除绘本（校验归属）
    static ServiceResult deleteBook(const std::string &userId, const std::string &bookId);
};
