#pragma once

#include <string>
#include "ServiceResult.h"

/** 图片二进制读取结果（供 raw / download 接口使用）。 */
struct ImageBinary {
    bool ok = false;
    int code = 500;
    std::string message;
    std::string content;
    std::string mimeType = "application/octet-stream";
    std::string filename;
};

/** 生成图业务逻辑层。 */
class ImageService {
public:
    /** 图片详情（校验归属）。 */
    static ServiceResult getImage(const std::string &userId, const std::string &imageId);

    /** 当前用户图片分页列表（bookId 为空表示不筛选）。 */
    static ServiceResult listImages(const std::string &userId,
                                    int page,
                                    int limit,
                                    const std::string &bookId);

    /** 删除图片（校验归属，并同步删除物理文件）。 */
    static ServiceResult deleteImage(const std::string &userId, const std::string &imageId);

    /**
     * 读取图片二进制。
     *
     * 该接口供前端 `<img src>` 预览使用，故**不校验归属**——
     * 图片 ID 为 UUID，属于"不可枚举即不可达"的公开资源；
     * 需要归属校验的下载/删除接口走上面的 Service 方法。
     */
    static ImageBinary loadImageFile(const std::string &imageId);
};
