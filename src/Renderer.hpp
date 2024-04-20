#pragma once

#include <SDL2/SDL.h>

#include "render/RenderQueue.hpp"
#include "render/Text.hpp"
#include "resources/Configs.hpp"
#include "resources/Resources.hpp"

#include <string>

namespace sge {

class Renderer {
public:
    Renderer() = delete;

    static void initialize(const std::string &windowTitle, resources::RenderingConfig config,
                           const std::string &fontName);

    static SDL_Window* globalWindow();
    static SDL_Renderer* globalRenderer();
    static const std::string &globalFontName();
    static int windowWidth();
    static int windowHeight();

    static void renderClear();

    static void renderUiImage(const resources::Image &img, const SDL_Point &pos);
    static void renderUiImageEx(const resources::Image &img, const SDL_Point &pos,
                                const SDL_Color &color);
    static void renderImage(const resources::Image &img, float x, float y,
                            const glm::vec2 &cameraPos, float zoom);
    static void renderImageEx(const render::DrawImageExArgs &args, const glm::vec2 &cameraPos,
                              float zoom);

    static void renderText(const render::Text &text, int x, int y);

    static void renderPixel(const SDL_Point &location, const SDL_Color &color);

    static void present();
};

}; // namespace sge
