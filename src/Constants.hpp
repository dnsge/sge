#pragma once

#include <SDL2/SDL.h>

#include <string_view>

namespace sge {

// Number of audio channels to allocate
constexpr int AudioChannelCount = 50;

// Display pixels per scene meter
constexpr float PixelsPerMeter = 100.0F;

// Default font size for rendering text
constexpr int DefaultFontSize = 16;

// Default color for rendering text
constexpr auto DefaultTextColor = SDL_Color{255, 255, 255, 255};

namespace events {

using namespace std::string_view_literals;

// Event invoked when client joins
constexpr auto MultiplayerOnClientJoin = "sge.Multiplayer.OnClientJoin"sv;

// Event invoked when client leaves
constexpr auto MultiplayerOnClientLeave = "sge.Multiplayer.OnClientLeave"sv;

} // namespace events

} // namespace sge
