#include "FileUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::vector<std::string> split(const std::string &text, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

}  // namespace

std::string FileUtils::joinPath(const std::string &base, const std::string &relative) {
    if (base.empty()) {
        return relative;
    }
    fs::path basePath(base);
    return (basePath / relative).lexically_normal().string();
}

bool FileUtils::isSafeRelativePath(const std::string &relative) {
    if (relative.empty()) {
        return false;
    }
    // 绝对路径直接拒绝
    if (relative.front() == '/') {
        return false;
    }
    // Windows 风格分隔符与盘符一律拒绝（避免跨平台语义歧义）
    if (relative.find('\\') != std::string::npos) {
        return false;
    }
    for (const auto &segment : split(relative, '/')) {
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
    }
    return true;
}

std::string FileUtils::extensionOf(const std::string &filename) {
    const auto dot = filename.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= filename.size()) {
        return "";
    }
    std::string ext = filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string FileUtils::mimeTypeOf(const std::string &extension) {
    const std::string ext = extensionOf("x." + extension);
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "webp") return "image/webp";
    if (ext == "gif") return "image/gif";
    if (ext == "pdf") return "application/pdf";
    return "application/octet-stream";
}

long long FileUtils::fileSize(const std::string &path) {
    try {
        if (!fs::exists(path) || !fs::is_regular_file(path)) {
            return -1;
        }
        return static_cast<long long>(fs::file_size(path));
    } catch (const std::exception &) {
        return -1;
    }
}

bool FileUtils::readFile(const std::string &path, std::string &out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

bool FileUtils::removeFile(const std::string &path) {
    try {
        if (!fs::exists(path)) {
            return true;
        }
        return fs::remove(path);
    } catch (const std::exception &) {
        return false;
    }
}

bool FileUtils::ensureParentDir(const std::string &path) {
    try {
        const fs::path parent = fs::path(path).parent_path();
        if (parent.empty()) {
            return true;
        }
        return fs::create_directories(parent);
    } catch (const std::exception &) {
        return false;
    }
}
