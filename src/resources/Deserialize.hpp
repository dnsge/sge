#pragma once

#include <rapidjson/document.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace sge {

void ReadJsonFile(const std::filesystem::path &path, rapidjson::Document &outDocument);

/**
 * @brief Safely get the value of a key in a JSON document.
 * 
 * @tparam T Type of value expected.
 * @tparam Doc Type of rapidjson document
 * @param doc Document to inspect.
 * @param key Key to get value of.
 * @return Value, or std::nullopt if invalid.
 */
template <typename T, typename Doc>
std::optional<T> GetKeySafe(const Doc &doc, const char* key) {
    // Use const char* for std::string initialization
    typedef std::conditional_t<std::is_same_v<T, std::string>, const char*, T> Val;

    if (!doc.HasMember(key)) {
        return std::nullopt;
    }
    const auto &val = doc[key];

    if constexpr (std::is_arithmetic_v<Val> && !std::is_same_v<Val, bool>) {
        // Number can convert to arithmetic types
        if (!val.IsNumber()) {
            return std::nullopt;
        }
    } else {
        if (!val.template Is<Val>()) {
            return std::nullopt;
        }
    }

    return {val.template Get<Val>()};
}

template <typename Doc>
std::optional<rapidjson::Value::ConstArray> GetArraySafe(const Doc &doc, const char* key) {
    if (!doc.HasMember(key)) {
        return std::nullopt;
    }
    const auto &val = doc[key];
    if (!val.IsArray()) {
        return std::nullopt;
    }
    return std::optional{val.GetArray()};
}

template <typename Doc>
std::optional<rapidjson::Value::ConstObject> GetObjectSafe(const Doc &doc, const char* key) {
    if (!doc.HasMember(key)) {
        return std::nullopt;
    }
    const auto &val = doc[key];
    if (!val.IsObject()) {
        return std::nullopt;
    }
    return std::optional{val.GetObject()};
}

/**
 * @brief Get the value of a key in a JSON document, or the zero-value if it 
 * does not exist.
 * 
 * @tparam T Type of value expected.
 * @tparam Doc Type of rapidjson document
 * @param doc Document to inspect.
 * @param key Key to get value of.
 * @return Value, or the zero-value.
 */
template <typename T, typename Doc>
T GetKeyOrZero(const Doc &doc, const char* key) {
    return GetKeySafe<T, Doc>(doc, key).value_or(T{});
}

template <typename Doc>
std::vector<std::string> GetStringArraySafe(const Doc &doc, const char* key) {
    const auto arr = GetArraySafe(doc, key);
    if (!arr) {
        return {};
    }

    std::vector<std::string> res;
    res.reserve(arr->Size());

    for (size_t i = 0; i < arr->Size(); ++i) {
        const auto &elem = (*arr)[i];
        if (elem.IsString()) {
            res.emplace_back(elem.GetString());
        }
    }

    return res;
}

using ComponentValueType = std::variant<std::string, int, float, bool>;

std::optional<ComponentValueType> ParseComponentValueType(const rapidjson::Value &val);

} // namespace sge
