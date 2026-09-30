#include "PageService.h"

#include <spdlog/spdlog.h>

#include "../models/BookPage.h"
#include "../repositories/BookPageRepository.h"
#include "../repositories/BookRepository.h"
#include "../repositories/GeneratedImageRepository.h"

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

}  // namespace

ServiceResult PageService::savePage(const std::string &userId,
                                    const std::string &bookId,
                                    const SavePageRequest &req) {
    ServiceResult result;

    // 1. 绘本归属校验
    auto book = BookRepository::findById(bookId);
    if (!book.has_value()) {
        return notFound("绘本不存在");
    }
    if (book->userId != userId) {
        return forbidden("无权操作该绘本");
    }

    // 2. 图片归属校验（非本人图片一律按"不存在"处理，避免枚举）
    auto image = GeneratedImageRepository::findByIdAndUser(req.imageId, userId);
    if (!image.has_value()) {
        return notFound("图片不存在");
    }

    // 3. upsert 页
    BookPage page;
    page.bookId = bookId;
    page.pageNumber = req.pageNumber;
    if (!req.title.empty()) {
        page.title = req.title;
    }
    page.imageId = req.imageId;

    if (!BookPageRepository::upsert(page)) {
        result.code = 500;
        result.message = "保存到绘本失败";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "保存成功";
    result.data = page.toJson();
    return result;
}

ServiceResult PageService::listPages(const std::string &userId, const std::string &bookId) {
    ServiceResult result;

    auto book = BookRepository::findById(bookId);
    if (!book.has_value()) {
        return notFound("绘本不存在");
    }
    if (book->userId != userId) {
        return forbidden("无权访问该绘本");
    }

    const auto pages = BookPageRepository::findByBookId(bookId);
    json items = json::array();
    for (const auto &page : pages) {
        items.push_back(page.toJson());
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = items;
    return result;
}
