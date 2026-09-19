#pragma once

#include <string>

/**
 * 密码单向哈希工具（加密存储核心）
 *
 * 支持两种算法，哈希串自描述，验证时自动识别，便于后续算法升级：
 *   - bcrypt        "$2a$"/"$2b$"/"$2y$"          —— 编译期检测到 libbcrypt 时启用（首选）
 *   - pbkdf2_sha256 "pbkdf2_sha256$iter$salt$hash" —— OpenSSL 兜底，始终可用
 *
 * 严禁：明文存储、可逆加密存储密码、MD5/SHA1 直接存储。
 */
class PasswordUtils {
public:
    /** 生成密码哈希（按编译期可用算法自动选择） */
    static std::string hashPassword(const std::string &password);

    /** 校验密码（依据哈希串前缀自动识别算法） */
    static bool verifyPassword(const std::string &password, const std::string &hash);

    /** 强度校验：≥8 位且同时包含大小写字母与数字 */
    static bool isStrongPassword(const std::string &password);

    /** 当前生效的算法名，用于日志与接口说明 */
    static std::string algorithmName();

    /** 恒定时间比较，避免时序侧信道 */
    static bool constantTimeEquals(const std::string &lhs, const std::string &rhs);

private:
    static std::string pbkdf2Hash(const std::string &password);
    static bool verifyPbkdf2(const std::string &password, const std::string &hash);

#if defined(SC_HAVE_BCRYPT)
    static std::string bcryptHash(const std::string &password);
    static bool verifyBcrypt(const std::string &password, const std::string &hash);
#endif
};
