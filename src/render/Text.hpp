#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "util/SDLPtr.hpp"

#include <string>
#include <string_view>

namespace sge {

class Renderer;

namespace render {

class Text {
public:
    Text(std::string_view text, std::string_view fontName, int fontSize, SDL_Color color);
    Text(std::string_view text, int fontSize);
    Text(std::string_view text);

    int width() const;
    int height() const;
    bool validFont() const;

private:
    friend class sge::Renderer;

    std::string text_;
    SDL_Color color_;
    TTF_Font* font_;
    util::SurfacePtr surface_;
    util::TexturePtr texture_;
};

} // namespace render

} // namespace sge
