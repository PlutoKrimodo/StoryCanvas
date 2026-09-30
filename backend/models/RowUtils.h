#pragma once

#include <nlohmann/json.hpp>
#include <drogon/orm/Field.h>

#include <optional>
#include <string>

using json = nlohmann::json;

/**
 * 数据库行 → C++ 类型的公共辅助函数。
 *
 * 统一口径（延续开发记录问题 13 / 15）：
 *   - 空串与 NULL 一律视为"无值"，避免同一字段出现两种空表示；
 *   - 可能为空的字段序列化时显式构造 json，规避问题 7 的类型推导陷阱。
 */
namespace rowutil {

inline std::optional<std::string> optString(const drogon::orm::Field &field) {
    if (field.isNull()) {
        return std::nullopt;
    }
    std::string value(field.as<std::string_view>());
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

inline std::string str(const drogon::orm::Field &field) {
    if (field.isNull()) {
        return "";
    }
    return std::string(field.as<std::string_view>());
}

inline std::optional<int> optInt(const drogon::orm::Field &field) {
    if (field.isNull()) {
        return std::nullopt;
    }
    return field.as<int>();
}

inline std::optional<long long> optLong(const drogon::orm::Field &field) {
    if (field.isNull()) {
        return std::nullopt;
    }
    return field.as<long long>();
}

inline std::optional<json> optJson(const drogon::orm::Field &field) {
    if (field.isNull()) {
        return std::nullopt;
    }
    const std::string text(field.as<std::string_view>());
    if (text.empty()) {
        return std::nullopt;
    }
    try {
        return json::parse(text);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

inline json jsonObjectOrEmpty(const drogon::orm::Field &field) {
    auto parsed = optJson(field);
    if (parsed.has_value() && parsed->is_object()) {
        return *parsed;
    }
    return json::object();
}

/**
 * 把 optional<T> 序列化为 json（有值 → 值，无值 → null）。
 *
 * 显式构造 json 而非用三元表达式，规避 `cond ? nullptr : T` 被推导为
 * 非 json 类型后崩溃的陷阱（见开发记录问题 7）。
 */
template <typename T>
inline json toJsonValue(const std::optional<T> &value) {
    if (value.has_value()) {
        return json(*value);
    }
    return json(nullptr);
}

}  // namespace rowutil
