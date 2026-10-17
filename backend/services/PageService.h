#pragma once

#include <string>
#include "ServiceResult.h"
#include "../dto/PageDto.h"

/** 页容器业务逻辑层：负责「保存到绘本」与「读取页列表」。 */
class PageService {
public:
    /** 保存某张生成图为绘本的一页（存在则更新标题与绑定）。 */
    static ServiceResult savePage(const std::string &userId,
                                  const std::string &bookId,
                                  const SavePageRequest &req);

    /** 整组图片按顺序写入连续页；全部成功或全部不写。 */
    static ServiceResult saveBatchPages(const std::string &userId,
                                        const std::string &bookId,
                                        const SaveBatchPagesRequest &req);

    /** 按页码升序返回绘本的所有页（供前端合成 PDF）。 */
    static ServiceResult listPages(const std::string &userId, const std::string &bookId);
};
