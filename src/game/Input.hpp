#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sge::game {

class Input {
public:
    enum class ButtonState {
        Up,
        JustDown,
        Down,
        JustUp
    };

    enum MouseButton : uint8_t {
        Left = 1,
        Middle = 2,
        Right = 3
    };

    static void init();

    /**
     * @brief Read all pending SDL events into the internal event buffer.
     * 
     * @return true When a Quit event was received 
     * @return false When no Quit event was received
     */
    static bool loadPendingEvents();

    /**
     * @brief Check whether a key is pressed down.
     * 
     * @param scancode Keyboard scancode to check for.
     */
    static bool getKey(SDL_Scancode scancode);
    static bool getKey(std::string_view keycode);

    /**
     * @brief Check whether a key was just pressed down.
     * 
     * @param scancode Keyboard scancode to check for.
     */
    static bool getKeyDown(SDL_Scancode scancode);
    static bool getKeyDown(std::string_view keycode);

    /**
     * @brief Check whether a key was just released.
     * 
     * @param scancode Keyboard scancode to check for.
     */
    static bool getKeyUp(SDL_Scancode scancode);
    static bool getKeyUp(std::string_view keycode);

    static ButtonState keyState(SDL_Scancode scancode);

    static bool getMouseButton(MouseButton button);
    static bool getMouseButton(int button);

    static bool getMouseButtonDown(MouseButton button);
    static bool getMouseButtonDown(int button);

    static bool getMouseButtonUp(MouseButton button);
    static bool getMouseButtonUp(int button);

    static ButtonState mouseState(MouseButton button);

    static const glm::ivec2 &mousePosition();
    static float mouseScroll();

private:
    static void transitionJustStates();
    static void processKey(SDL_Scancode scancode, bool down);
    static void processMouseButton(MouseButton button, bool down);

    static std::optional<MouseButton> parseMouseButton(int num);

    static std::array<ButtonState, SDL_NUM_SCANCODES> KeyStates;
    static std::array<ButtonState, 3> MouseButtonStates;
    static glm::ivec2 MousePosition;
    static float MouseScroll;
};

} // namespace sge::game
