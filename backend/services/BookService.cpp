#include "BookService.h"
#include "../repositories/BookRepository.h"
#include <spdlog/spdlog.h>

namespace {

ServiceResult notFound() {
    ServiceResult result;
    result.code = 404;
    result.message = "绘本不存在";
    return result;
}

ServiceResult forbidden(const std::string &message) {
    ServiceResult result;
    result.code = 403;
    result.message = message;
    return result;
}

}  // namespace

ServiceResult BookService::createBook(const std::string &userId, const CreateBookRequest &req) {
    ServiceResult result;

    Book book;
    book.userId = userId;
    book.title = req.title;
    if (!req.description.empty()) {
        book.description = req.description;
    }
    book.status = req.status.empty() ? "draft" : req.status;

    if (!BookRepository::create(book)) {
        result.code = 500;
        result.message = "创建绘本失败";
        return result;
    }

    result.success = true;
    result.code = 201;
    result.message = "创建成功";
    result.data = book.toJson();
    return result;
}

ServiceResult BookService::listBooks(const std::string &userId,
                                     int page,
                                     int limit,
                                     const std::string &status) {
    ServiceResult result;

    if (page < 1) page = 1;
    if (limit < 1) limit = 20;
    if (limit > 100) limit = 100;
    const int offset = (page - 1) * limit;

    const auto books = BookRepository::findByUserId(userId, status, limit, offset);
    const int total = BookRepository::countByUserId(userId, status);

    json items = json::array();
    for (const auto &book : books) {
        items.push_back(book.toJson());
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = {
        {"items", items},
        {"total", total},
        {"page", page},
        {"limit", limit}
    };
    return result;
}

ServiceResult BookService::getBook(const std::string &userId, const std::string &bookId) {
    ServiceResult result;

    auto bookOpt = BookRepository::findById(bookId);
    if (!bookOpt) {
        return notFound();
    }
    if (bookOpt->userId != userId) {
        return forbidden("无权访问该绘本");
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = bookOpt->toJson();
    return result;
}

ServiceResult BookService::updateBook(const std::string &userId,
                                      const std::string &bookId,
                                      const UpdateBookRequest &req) {
    ServiceResult result;

    auto bookOpt = BookRepository::findById(bookId);
    if (!bookOpt) {
        return notFound();
    }
    Book &book = *bookOpt;
    if (book.userId != userId) {
        return forbidden("无权修改该绘本");
    }

    book.title = req.title;
    book.description = req.description.empty()
                           ? std::nullopt
                           : std::optional<std::string>(req.description);
    if (!req.status.empty()) {
        book.status = req.status;
    }

    if (!BookRepository::update(book)) {
        result.code = 500;
        result.message = "更新绘本失败";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "更新成功";
    result.data = book.toJson();
    return result;
}

ServiceResult BookService::deleteBook(const std::string &userId, const std::string &bookId) {
    ServiceResult result;

    auto bookOpt = BookRepository::findById(bookId);
    if (!bookOpt) {
        return notFound();
    }
    if (bookOpt->userId != userId) {
        return forbidden("无权删除该绘本");
    }

    if (!BookRepository::deleteById(bookId)) {
        result.code = 500;
        result.message = "删除绘本失败";
        return result;
    }

    result.success = true;
    result.code = 200;
    result.message = "删除成功";
    return result;
}
