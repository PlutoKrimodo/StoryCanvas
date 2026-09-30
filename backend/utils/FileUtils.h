#pragma once

#include <string>

/**
 * 文件工具（纯函数，无 Drogon 依赖）。
 *
 * 之所以保持"零依赖"，是因为 backend/tests 会 glob 编译整个 utils/ 目录，
 * 单元测试不方便链接 Drogon（见 tests/CMakeLists.txt）。
 */
class FileUtils {
public:
    /** 拼接 base 与 relative 为绝对路径（会做词法归一，不做存在性检查）。 */
    static std::string joinPath(const std::string &base, const std::string &relative);

    /**
     * 判断相对路径是否安全：
     * 拒绝绝对路径、`..` 越界、空段与反斜杠，用于防御路径穿越。
     */
    static bool isSafeRelativePath(const std::string &relative);

    /** 取小写扩展名（不含点）；无扩展名返回空串。 */
    static std::string extensionOf(const std::string &filename);

    /** 由扩展名推断 MIME 类型，未知时回退 application/octet-stream。 */
    static std::string mimeTypeOf(const std::string &extension);

    /** 文件大小（字节）；文件不存在或读取失败返回 -1。 */
    static long long fileSize(const std::string &path);

    /** 读取整个文件到 out；成功返回 true。 */
    static bool readFile(const std::string &path, std::string &out);

    /** 删除文件；成功或文件不存在均视为成功。 */
    static bool removeFile(const std::string &path);

    /** 确保 path 的父目录存在（用于落盘前建档）。 */
    static bool ensureParentDir(const std::string &path);
};
