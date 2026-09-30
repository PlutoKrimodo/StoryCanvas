#include "BookPage.h"

#include "RowUtils.h"

namespace {
constexpr const char *kImageRawPrefix = "/api/v1/images/";
constexpr const char *kImageRawSuffix = "/raw";
}  // namespace

json BookPage::toJson() const {
    json imageUrlValue = nullptr;
    if (imageId.has_value()) {
        imageUrlValue = std::string(kImageRawPrefix) + *imageId + kImageRawSuffix;
    }

    return {
        {"id", id},
        {"book_id", bookId},
        {"page_number", pageNumber},
        {"title", rowutil::toJsonValue(title)},
        {"thumbnail", rowutil::toJsonValue(thumbnail)},
        {"image_id", rowutil::toJsonValue(imageId)},
        {"image_url", imageUrlValue},
        {"created_at", rowutil::toJsonValue(createdAt)},
        {"updated_at", rowutil::toJsonValue(updatedAt)}
    };
}

BookPage BookPage::fromRow(const drogon::orm::Row &row) {
    BookPage page;
    page.id = rowutil::str(row["id"]);
    page.bookId = rowutil::str(row["book_id"]);
    if (!row["page_number"].isNull()) {
        page.pageNumber = row["page_number"].as<int>();
    }
    page.title = rowutil::optString(row["title"]);
    page.thumbnail = rowutil::optString(row["thumbnail"]);
    page.imageId = rowutil::optString(row["image_id"]);
    page.imageFilePath = rowutil::optString(row["image_file_path"]);
    page.createdAt = rowutil::optString(row["created_at"]);
    page.updatedAt = rowutil::optString(row["updated_at"]);
    return page;
}
