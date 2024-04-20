#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace detail {

struct StringHash {
    using is_transparent = void;

    [[nodiscard]] std::size_t operator()(const char* txt) const {
        return std::hash<std::string_view>{}(txt);
    }

    [[nodiscard]] std::size_t operator()(std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }

    [[nodiscard]] std::size_t operator()(const std::string &txt) const {
        return std::hash<std::string>{}(txt);
    }
};

} // namespace detail

template <typename V>
using unordered_string_map =
    std::unordered_map<std::string, V, detail::StringHash, std::equal_to<>>;
