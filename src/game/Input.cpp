#include "game/Input.hpp"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "gea/Helper.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sge::game {

namespace {

using namespace std::string_view_literals;

const std::unordered_map<std::string_view, SDL_Scancode> KeycodeToScancode = {
  // Directional (arrow) Keys
    {       "up"sv,           SDL_SCANCODE_UP},
    {     "down"sv,         SDL_SCANCODE_DOWN},
    {    "right"sv,        SDL_SCANCODE_RIGHT},
    {     "left"sv,         SDL_SCANCODE_LEFT},

 // Misc Keys
    {   "escape"sv,       SDL_SCANCODE_ESCAPE},

 // Modifier Keys
    {   "lshift"sv,       SDL_SCANCODE_LSHIFT},
    {   "rshift"sv,       SDL_SCANCODE_RSHIFT},
    {    "lctrl"sv,        SDL_SCANCODE_LCTRL},
    {    "rctrl"sv,        SDL_SCANCODE_RCTRL},
    {     "lalt"sv,         SDL_SCANCODE_LALT},
    {     "ralt"sv,         SDL_SCANCODE_RALT},

 // Editing Keys
    {      "tab"sv,          SDL_SCANCODE_TAB},
    {   "return"sv,       SDL_SCANCODE_RETURN},
    {    "enter"sv,       SDL_SCANCODE_RETURN},
    {"backspace"sv,    SDL_SCANCODE_BACKSPACE},
    {   "delete"sv,       SDL_SCANCODE_DELETE},
    {   "insert"sv,       SDL_SCANCODE_INSERT},

 // Character Keys
    {    "space"sv,        SDL_SCANCODE_SPACE},
    {        "a"sv,            SDL_SCANCODE_A},
    {        "b"sv,            SDL_SCANCODE_B},
    {        "c"sv,            SDL_SCANCODE_C},
    {        "d"sv,            SDL_SCANCODE_D},
    {        "e"sv,            SDL_SCANCODE_E},
    {        "f"sv,            SDL_SCANCODE_F},
    {        "g"sv,            SDL_SCANCODE_G},
    {        "h"sv,            SDL_SCANCODE_H},
    {        "i"sv,            SDL_SCANCODE_I},
    {        "j"sv,            SDL_SCANCODE_J},
    {        "k"sv,            SDL_SCANCODE_K},
    {        "l"sv,            SDL_SCANCODE_L},
    {        "m"sv,            SDL_SCANCODE_M},
    {        "n"sv,            SDL_SCANCODE_N},
    {        "o"sv,            SDL_SCANCODE_O},
    {        "p"sv,            SDL_SCANCODE_P},
    {        "q"sv,            SDL_SCANCODE_Q},
    {        "r"sv,            SDL_SCANCODE_R},
    {        "s"sv,            SDL_SCANCODE_S},
    {        "t"sv,            SDL_SCANCODE_T},
    {        "u"sv,            SDL_SCANCODE_U},
    {        "v"sv,            SDL_SCANCODE_V},
    {        "w"sv,            SDL_SCANCODE_W},
    {        "x"sv,            SDL_SCANCODE_X},
    {        "y"sv,            SDL_SCANCODE_Y},
    {        "z"sv,            SDL_SCANCODE_Z},
    {        "0"sv,            SDL_SCANCODE_0},
    {        "1"sv,            SDL_SCANCODE_1},
    {        "2"sv,            SDL_SCANCODE_2},
    {        "3"sv,            SDL_SCANCODE_3},
    {        "4"sv,            SDL_SCANCODE_4},
    {        "5"sv,            SDL_SCANCODE_5},
    {        "6"sv,            SDL_SCANCODE_6},
    {        "7"sv,            SDL_SCANCODE_7},
    {        "8"sv,            SDL_SCANCODE_8},
    {        "9"sv,            SDL_SCANCODE_9},
    {        "/"sv,        SDL_SCANCODE_SLASH},
    {        ";"sv,    SDL_SCANCODE_SEMICOLON},
    {        "="sv,       SDL_SCANCODE_EQUALS},
    {        "-"sv,        SDL_SCANCODE_MINUS},
    {        "."sv,       SDL_SCANCODE_PERIOD},
    {        ","sv,        SDL_SCANCODE_COMMA},
    {        "["sv,  SDL_SCANCODE_LEFTBRACKET},
    {        "]"sv, SDL_SCANCODE_RIGHTBRACKET},
    {       R"(\)",    SDL_SCANCODE_BACKSLASH},
    {        "'"sv,   SDL_SCANCODE_APOSTROPHE}
};

} // namespace

std::array<Input::ButtonState, SDL_NUM_SCANCODES> Input::KeyStates = {};
std::array<Input::ButtonState, 3> Input::MouseButtonStates = {};
glm::ivec2 Input::MousePosition = glm::ivec2{0, 0};
float Input::MouseScroll = 0.0F;

void Input::init() {
    Input::KeyStates.fill(ButtonState::Up);
    Input::MouseButtonStates.fill(ButtonState::Up);
}

bool Input::loadPendingEvents() {
    // Clear mouse scroll before frame
    Input::MouseScroll = 0.0F;

    // Convert JustUp -> Up and JustDown -> Down
    Input::transitionJustStates();

    // Process events in queue
    SDL_Event event;
    while (Helper::SDL_PollEvent498(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            return true;
        case SDL_KEYDOWN:
            Input::processKey(event.key.keysym.scancode, true);
            break;
        case SDL_KEYUP:
            Input::processKey(event.key.keysym.scancode, false);
            break;
        case SDL_MOUSEBUTTONDOWN:
            Input::processMouseButton(static_cast<MouseButton>(event.button.button), true);
            break;
        case SDL_MOUSEBUTTONUP:
            Input::processMouseButton(static_cast<MouseButton>(event.button.button), false);
            break;
        case SDL_MOUSEMOTION:
            Input::MousePosition.x = event.motion.x;
            Input::MousePosition.y = event.motion.y;
            break;
        case SDL_MOUSEWHEEL:
            Input::MouseScroll = event.wheel.preciseY;
            break;
        default:
            break;
        }
    }

    // No quit
    return false;
}

bool Input::getKey(SDL_Scancode scancode) {
    auto state = Input::keyState(scancode);
    return state == Input::ButtonState::JustDown || state == Input::ButtonState::Down;
}

bool Input::getKey(std::string_view keycode) {
    auto scancode = KeycodeToScancode.find(keycode);
    if (scancode == KeycodeToScancode.end()) {
        return false;
    }
    return Input::getKey(scancode->second);
}

bool Input::getKeyDown(SDL_Scancode scancode) {
    auto state = Input::keyState(scancode);
    return state == Input::ButtonState::JustDown;
}

bool Input::getKeyDown(std::string_view keycode) {
    auto scancode = KeycodeToScancode.find(keycode);
    if (scancode == KeycodeToScancode.end()) {
        return false;
    }
    return Input::getKeyDown(scancode->second);
}

bool Input::getKeyUp(SDL_Scancode scancode) {
    auto state = Input::keyState(scancode);
    return state == Input::ButtonState::JustUp;
}

bool Input::getKeyUp(std::string_view keycode) {
    auto scancode = KeycodeToScancode.find(keycode);
    if (scancode == KeycodeToScancode.end()) {
        return false;
    }
    return Input::getKeyUp(scancode->second);
}

Input::ButtonState Input::keyState(SDL_Scancode scancode) {
    return Input::KeyStates[scancode];
}

void Input::transitionJustStates() {
    constexpr auto TransitionState = [](ButtonState &state) -> void {
        if (state == Input::ButtonState::JustDown) {
            state = Input::ButtonState::Down;
        } else if (state == Input::ButtonState::JustUp) {
            state = Input::ButtonState::Up;
        }
    };

    // Transition key states
    std::for_each(KeyStates.begin(), KeyStates.end(), TransitionState);
    // Transition mouse button states
    std::for_each(MouseButtonStates.begin(), MouseButtonStates.end(), TransitionState);
}

void Input::processKey(SDL_Scancode scancode, bool down) {
    using ButtonState = Input::ButtonState;
    auto &state = Input::KeyStates[scancode];
    state = down ? ButtonState::JustDown : ButtonState::JustUp;
}

void Input::processMouseButton(MouseButton button, bool down) {
    using ButtonState = Input::ButtonState;
    auto index = static_cast<size_t>(button) - 1;
    auto &state = Input::MouseButtonStates[index];
    if (state == ButtonState::Down && !down) {
        state = ButtonState::JustUp;
    } else if (state == ButtonState::Up && down) {
        state = ButtonState::JustDown;
    }
}

bool Input::getMouseButton(MouseButton button) {
    auto state = Input::mouseState(button);
    return state == Input::ButtonState::JustDown || state == Input::ButtonState::Down;
}

bool Input::getMouseButton(int button) {
    if (auto mouseButton = Input::parseMouseButton(button); mouseButton) {
        return Input::getMouseButton(*mouseButton);
    }
    return false;
}

bool Input::getMouseButtonDown(MouseButton button) {
    auto state = Input::mouseState(button);
    return state == Input::ButtonState::JustDown;
}

bool Input::getMouseButtonDown(int button) {
    if (auto mouseButton = Input::parseMouseButton(button); mouseButton) {
        return Input::getMouseButtonDown(*mouseButton);
    }
    return false;
}

bool Input::getMouseButtonUp(MouseButton button) {
    auto state = Input::mouseState(button);
    return state == Input::ButtonState::JustUp;
}

bool Input::getMouseButtonUp(int button) {
    if (auto mouseButton = Input::parseMouseButton(button); mouseButton) {
        return Input::getMouseButtonUp(*mouseButton);
    }
    return false;
}

Input::ButtonState Input::mouseState(MouseButton button) {
    auto index = static_cast<size_t>(button) - 1;
    return Input::MouseButtonStates[index];
}

std::optional<Input::MouseButton> Input::parseMouseButton(int num) {
    switch (num) {
    case 1:
        return MouseButton::Left;
    case 2:
        return MouseButton::Middle;
    case 3:
        return MouseButton::Right;
    default:
        return std::nullopt;
    }
}

const glm::ivec2 &Input::mousePosition() {
    return Input::MousePosition;
}

float Input::mouseScroll() {
    return Input::MouseScroll;
}

} // namespace sge::game
