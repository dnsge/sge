#include "resources/Deserialize.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <rapidjson/filereadstream.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

constexpr auto StreamBufferSize = 65536;

namespace sge {

void ReadJsonFile(const std::filesystem::path &path, rapidjson::Document &outDocument) {
    FILE* filePointer = nullptr;
#ifdef _WIN32
    fopen_s(&filePointer, path.string().c_str(), "rb");
#else
    filePointer = fopen(path.c_str(), "rb");
#endif

    std::array<char, StreamBufferSize> buffer;
    rapidjson::FileReadStream stream(filePointer, buffer.data(), buffer.size());
    outDocument.ParseStream(stream);
    std::fclose(filePointer);

    if (outDocument.HasParseError()) {
        std::cout << "error parsing json at [" << path << "]" << std::endl;
        std::exit(0);
    }
}

std::optional<ComponentValueType> ParseComponentValueType(const rapidjson::Value &val) {
    if (val.IsString()) {
        return {std::string(val.GetString())};
    } else if (val.IsInt()) {
        return {val.GetInt()};
    } else if (val.IsFloat()) {
        return {val.GetFloat()};
    } else if (val.IsBool()) {
        return {val.GetBool()};
    }
    return std::nullopt;
}

} // namespace sge
