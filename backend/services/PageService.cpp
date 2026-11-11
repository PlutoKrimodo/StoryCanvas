#include "PageService.h"

#include <spdlog/spdlog.h>

#include "../models/BookPage.h"
#include "../models/GeneratedImage.h"
#include "../repositories/BookPageRepository.h"
#include "../repositories/BookRepository.h"
#include "../repositories/GeneratedImageRepository.h"

#include <map>

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

ServiceResult contentReviewBlocked(const std::string &message) {
    ServiceResult result;
    result.code = 422;
    result.errorCode = "CONTENT_REVIEW_REQUIRED";
    result.message = message;
    return result;
}

bool isContentApproved(const GeneratedImage &image) {
    // 旧数据没有该字段时按历史行为放行；新生成图由 AI 服务写入策略状态。
    if (!image.parameters.is_object()) {
        return true;
    }
    return image.parameters.value("content_review_status", std::string("approved")) ==
           "approved";
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
    if (!isContentApproved(*image)) {
        return contentReviewBlocked("图片内容待复核，暂不能加入绘本，请重新生成或更换描述");
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

ServiceResult PageService::saveBatchPages(const std::string &userId,
                                          const std::string &bookId,
                                          const SaveBatchPagesRequest &req) {
    ServiceResult result;

    auto book = BookRepository::findById(bookId);
    if (!book.has_value()) {
        return notFound("绘本不存在");
    }
    if (book->userId != userId) {
        return forbidden("无权操作该绘本");
    }

    constexpr int kMaxPagesPerBook = 50;
    if (BookPageRepository::countByBookId(bookId) +
            static_cast<int>(req.imageIds.size()) >
        kMaxPagesPerBook) {
        result.code = 422;
        result.message = "加入后超过单本绘本 50 页上限";
        return result;
    }

    auto images = GeneratedImageRepository::findByIdsAndUser(req.imageIds, userId);
    if (!images.has_value()) {
        result.code = 500;
        result.message = "查询生成图失败，请稍后重试";
        return result;
    }
    if (images->size() != req.imageIds.size()) {
        return notFound("部分图片不存在或无权使用");
    }
    for (const auto &image : *images) {
        if (!isContentApproved(image)) {
            return contentReviewBlocked("部分图片内容待复核，暂不能加入绘本，请重新生成或更换描述");
        }
    }

    std::map<std::string, GeneratedImage> byId;
    for (const auto &image : *images) {
        byId[image.id] = image;
    }
    std::vector<GeneratedImage> ordered;
    ordered.reserve(req.imageIds.size());
    for (const auto &id : req.imageIds) {
        ordered.push_back(byId.at(id));
    }

    auto pages = BookPageRepository::insertBatch(bookId, ordered);
    if (!pages.has_value()) {
        result.code = 500;
        result.message = "批量保存到绘本失败";
        return result;
    }

    json items = json::array();
    for (const auto &page : *pages) {
        items.push_back(page.toJson());
    }

    result.success = true;
    result.code = 200;
    result.message = "已加入绘本";
    result.data = {{"items", items}};
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
