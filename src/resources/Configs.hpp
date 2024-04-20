#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace sge::resources {

constexpr unsigned int DefaultXResolution = 640;
constexpr unsigned int DefaultYResolution = 360;
constexpr unsigned int DefaultServerTickRate = 60;
constexpr unsigned int DefaultServerIoWorkers = 1;
constexpr int DefaultServerPort = 7462;

struct GameConfig {
    std::string window_title;
    std::string font;
};

struct RenderingConfig {
    unsigned int x_resolution;
    unsigned int y_resolution;

    unsigned int clear_color_r;
    unsigned int clear_color_g;
    unsigned int clear_color_b;

    glm::ivec2 size() const;
};

enum class ServerEmptyBehavior {
    Pause,
    Reset,
    Run,
};

constexpr ServerEmptyBehavior ServerEmptyBehaviorOfString(std::string_view s) {
    using namespace std::string_view_literals;
    if (s == "pause"sv) {
        return ServerEmptyBehavior::Pause;
    } else if (s == "reset"sv) {
        return ServerEmptyBehavior::Reset;
    } else if (s == "run"sv) {
        return ServerEmptyBehavior::Run;
    } else {
        return ServerEmptyBehavior::Reset;
    }
}

struct ServerConfig {
    unsigned int tick_rate;

    int port;
    unsigned int io_workers;
    ServerEmptyBehavior empty_behavior;

    std::string initial_scene;
};

struct ClientConfig {
    std::string initial_scene;
    std::optional<std::string> disconnected_scene;

    RenderingConfig rendering_config;
};

GameConfig LoadGameConfig();

ServerConfig LoadServerConfig();

ClientConfig LoadClientConfig();

} // namespace sge::resources
