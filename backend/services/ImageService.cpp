#include "ImageService.h"

#include <spdlog/spdlog.h>

#include "../repositories/GeneratedImageRepository.h"
#include "../utils/ConfigManager.h"
#include "../utils/FileUtils.h"

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

ServiceResult ImageService::getImage(const std::string &userId, const std::string &imageId) {
    ServiceResult result;

    auto image = GeneratedImageRepository::findById(imageId);
    if (!image.has_value()) {
        return notFound("图片不存在");
    }
    if (image->userId != userId) {
        return forbidden("无权访问该图片");
    }

    result.success = true;
    result.code = 200;
    result.message = "success";
    result.data = image->toJson();
    return result;
}

ServiceResult ImageService::listImages(const std::string &userId,
                                       int page,
                                       int limit,
                                       const std::string &bookId) {
    ServiceResult result;

    if (page < 1) page = 1;
    if (limit < 1) limit = 20;
    if (limit > 100) limit = 100;
    const int offset = (page - 1) * limit;

    const auto images = GeneratedImageRepository::findByUserId(userId, bookId, limit, offset);
    const int total = GeneratedImageRepository::countByUserId(userId, bookId);

    json items = json::array();
    for (const auto &image : images) {
        items.push_back(image.toJson());
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

ServiceResult ImageService::deleteImage(const std::string &userId, const std::string &imageId) {
    ServiceResult result;

    auto image = GeneratedImageRepository::findById(imageId);
    if (!image.has_value()) {
        return notFound("图片不存在");
    }
    if (image->userId != userId) {
        return forbidden("无权删除该图片");
    }

    if (!GeneratedImageRepository::deleteById(imageId)) {
        result.code = 500;
        result.message = "删除图片失败";
        return result;
    }

    // 物理文件删除为 best-effort：失败只记日志，不影响接口结果
    if (FileUtils::isSafeRelativePath(image->filePath)) {
        const std::string fullPath =
            FileUtils::joinPath(ConfigManager::storagePath, image->filePath);
        if (!FileUtils::removeFile(fullPath)) {
            spdlog::warn("删除生成图物理文件失败: {}", fullPath);
        }
    }

    result.success = true;
    result.code = 200;
    result.message = "删除成功";
    return result;
}

ImageBinary ImageService::loadImageFile(const std::string &imageId) {
    ImageBinary binary;

    auto image = GeneratedImageRepository::findById(imageId);
    if (!image.has_value()) {
        binary.code = 404;
        binary.message = "图片不存在";
        return binary;
    }

    // 防御路径穿越：file_path 必须是我们落库的安全相对路径
    if (!FileUtils::isSafeRelativePath(image->filePath)) {
        binary.code = 500;
        binary.message = "图片路径非法";
        spdlog::error("检测到非法 file_path: {}", image->filePath);
        return binary;
    }

    const std::string fullPath = FileUtils::joinPath(ConfigManager::storagePath, image->filePath);
    if (!FileUtils::readFile(fullPath, binary.content)) {
        binary.code = 404;
        binary.message = "图片文件不存在";
        return binary;
    }

    const std::string format =
        image->format.value_or(FileUtils::extensionOf(image->filePath));
    binary.ok = true;
    binary.code = 200;
    binary.mimeType = FileUtils::mimeTypeOf(format);
    binary.filename = "storycanvas-" + imageId + "." + (format.empty() ? "png" : format);
    return binary;
}
