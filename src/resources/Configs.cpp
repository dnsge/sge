#include "resources/Configs.hpp"

#include <rapidjson/document.h>

#include "resources/Deserialize.hpp"
#include "resources/Resources.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace sge::resources {

glm::ivec2 RenderingConfig::size() const {
    return {this->x_resolution, this->y_resolution};
}

GameConfig LoadGameConfig() {
    if (!std::filesystem::exists(GameConfigPath)) {
        std::cout << "error: " << GameConfigPath.string() << " missing" << std::endl;
        std::exit(0);
    }

    // Read config file JSON. Exits on parse error.
    rapidjson::Document doc;
    ReadJsonFile(GameConfigPath, doc);

    return GameConfig{
        .window_title = GetKeyOrZero<std::string>(doc, "window_title"),
        .font = GetKeyOrZero<std::string>(doc, "font"),
    };
}

ServerConfig LoadServerConfig() {
    if (!std::filesystem::exists(ServerConfigPath)) {
        std::cout << "error: " << ServerConfigPath.string() << " missing" << std::endl;
        std::exit(0);
    }

    rapidjson::Document doc;
    ReadJsonFile(ServerConfigPath, doc);

    auto initialScene = GetKeySafe<std::string>(doc, "initial_scene");
    if (!initialScene) {
        std::cout << "error: " << ServerConfigPath.string() << ": " << "initial_scene unspecified"
                  << std::endl;
        std::exit(0);
    }

    return ServerConfig{
        .tick_rate = GetKeySafe<unsigned int>(doc, "tick_rate").value_or(DefaultServerTickRate),

        .port = GetKeySafe<int>(doc, "port").value_or(DefaultServerPort),
        .io_workers = GetKeySafe<unsigned int>(doc, "io_workers").value_or(DefaultServerIoWorkers),
        .empty_behavior =
            ServerEmptyBehaviorOfString(GetKeyOrZero<std::string>(doc, "empty_behavior")),

        .initial_scene = std::move(*initialScene),
    };
}

RenderingConfig ParseRenderingConfig(const std::optional<rapidjson::Value::ConstObject> &obj) {
    if (!obj.has_value()) {
        return RenderingConfig{
            .x_resolution = DefaultXResolution,
            .y_resolution = DefaultYResolution,
            .clear_color_r = 255,
            .clear_color_g = 255,
            .clear_color_b = 255,
        };
    }

    return RenderingConfig{
        .x_resolution = GetKeySafe<unsigned int>(*obj, "x_resolution").value_or(DefaultXResolution),
        .y_resolution = GetKeySafe<unsigned int>(*obj, "y_resolution").value_or(DefaultYResolution),
        .clear_color_r = GetKeySafe<unsigned int>(*obj, "clear_color_r").value_or(255),
        .clear_color_g = GetKeySafe<unsigned int>(*obj, "clear_color_g").value_or(255),
        .clear_color_b = GetKeySafe<unsigned int>(*obj, "clear_color_b").value_or(255),
    };
}

ClientConfig LoadClientConfig() {
    if (!std::filesystem::exists(ClientConfigPath)) {
        std::cout << "error: " << ClientConfigPath.string() << " missing" << std::endl;
        std::exit(0);
    }

    rapidjson::Document doc;
    ReadJsonFile(ClientConfigPath, doc);

    auto initialScene = GetKeySafe<std::string>(doc, "initial_scene");
    if (!initialScene) {
        std::cout << "error: " << ClientConfigPath.string() << ": " << "initial_scene unspecified"
                  << std::endl;
        std::exit(0);
    }

    return ClientConfig{
        .initial_scene = std::move(*initialScene),
        .disconnected_scene = GetKeySafe<std::string>(doc, "disconnected_scene"),
        .rendering_config = ParseRenderingConfig(GetObjectSafe(doc, "rendering")),
    };
}

} // namespace sge::resources
