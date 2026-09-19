#pragma once

#include <string>

/**
 * Base64 / Base64URL 编解码工具
 *
 * - 标准 Base64（含 '=' 填充）：用于加密结果序列化
 * - Base64URL（'-' '_'，无填充）：用于 JWT 的 header / payload / signature
 */
class Base64 {
public:
    static std::string encode(const std::string &input);
    static std::string decode(const std::string &input);

    static std::string encodeUrl(const std::string &input);
    static std::string decodeUrl(const std::string &input);
};
