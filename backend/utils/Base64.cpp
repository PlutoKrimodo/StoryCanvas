#include "Base64.h"

#include <openssl/evp.h>

#include <stdexcept>

namespace {

std::string toUrlSafe(std::string value) {
    for (auto &ch : value) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }
    while (!value.empty() && value.back() == '=') {
        value.pop_back();
    }
    return value;
}

std::string fromUrlSafe(std::string value) {
    for (auto &ch : value) {
        if (ch == '-') {
            ch = '+';
        } else if (ch == '_') {
            ch = '/';
        }
    }
    while (value.size() % 4 != 0) {
        value.push_back('=');
    }
    return value;
}

}  // namespace

std::string Base64::encode(const std::string &input) {
    if (input.empty()) {
        return "";
    }

    std::string output;
    output.resize(4 * ((input.size() + 2) / 3));

    const int len = EVP_EncodeBlock(
        reinterpret_cast<unsigned char *>(&output[0]),
        reinterpret_cast<const unsigned char *>(input.data()),
        static_cast<int>(input.size()));
    if (len < 0) {
        throw std::runtime_error("Base64 编码失败");
    }
    output.resize(static_cast<size_t>(len));
    return output;
}

std::string Base64::decode(const std::string &input) {
    if (input.empty()) {
        return "";
    }

    std::string normalized = input;
    while (normalized.size() % 4 != 0) {
        normalized.push_back('=');
    }

    std::string output;
    output.resize(3 * (normalized.size() / 4));

    const int len = EVP_DecodeBlock(
        reinterpret_cast<unsigned char *>(&output[0]),
        reinterpret_cast<const unsigned char *>(normalized.data()),
        static_cast<int>(normalized.size()));
    if (len < 0) {
        throw std::runtime_error("Base64 解码失败");
    }

    // EVP_DecodeBlock 会把填充位也算作输出，这里按填充数裁剪
    size_t padding = 0;
    if (normalized.size() >= 1 && normalized[normalized.size() - 1] == '=') {
        ++padding;
    }
    if (normalized.size() >= 2 && normalized[normalized.size() - 2] == '=') {
        ++padding;
    }

    output.resize(static_cast<size_t>(len) - padding);
    return output;
}

std::string Base64::encodeUrl(const std::string &input) {
    return toUrlSafe(encode(input));
}

std::string Base64::decodeUrl(const std::string &input) {
    return decode(fromUrlSafe(input));
}
