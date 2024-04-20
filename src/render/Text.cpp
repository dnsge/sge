#include "render/Text.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Constants.hpp"
#include "Renderer.hpp"
#include "resources/Resources.hpp"
#include "util/SDLPtr.hpp"

#include <cassert>
#include <string>
#include <string_view>

namespace sge::render {

Text::Text(std::string_view text, std::string_view fontName, int fontSize, SDL_Color color)
    : text_(text) {
    if (fontName.empty()) {
        this->font_ = nullptr;
        this->surface_ = nullptr;
        this->texture_ = nullptr;
        return;
    }

    this->font_ = resources::GetFont(fontName, fontSize);
    this->surface_ =
        util::SurfacePtr{TTF_RenderText_Solid(this->font_, this->text_.c_str(), color)};
    this->texture_ = util::TexturePtr{
        SDL_CreateTextureFromSurface(Renderer::globalRenderer(), this->surface_.get())};

    assert(this->surface_ != nullptr);
    assert(this->texture_ != nullptr);
}

Text::Text(std::string_view text, int fontSize)
    : Text(text, Renderer::globalFontName(), fontSize, DefaultTextColor) {}

Text::Text(std::string_view text)
    : Text(text, Renderer::globalFontName(), DefaultFontSize, DefaultTextColor) {}

int Text::width() const {
    return this->surface_->w;
}

int Text::height() const {
    return this->surface_->h;
}

bool Text::validFont() const {
    return this->font_ != nullptr;
}

} // namespace sge::render
