#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../models/UserAiCredential.h"

/**
 * user_ai_credentials 数据访问。
 *
 * 约定与现有 Repository 一致：静态方法 + 同步 DbClient + 参数化 SQL；
 * 数据库异常统一记录日志并返回 nullopt / false，由服务层转成 500。
 */
class UserAiCredentialRepository {
public:
    /** 查询用户全部凭据；数据库异常返回 nullopt（与「确实没有」区分）。 */
    static std::optional<std::vector<UserAiCredential>> findByUser(const std::string &userId);

    static std::optional<UserAiCredential> findByUserAndCapability(const std::string &userId,
                                                                   const std::string &capability);

    /** 新增或更新（唯一键 user_id + capability）；成功后回填 id / 时间戳。 */
    static bool upsert(UserAiCredential &credential);

    /** 删除；返回是否确实删除了一行。 */
    static bool remove(const std::string &userId, const std::string &capability);
};
