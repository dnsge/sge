#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

#include <memory>

namespace sge::util {

struct SurfaceDeleter {
    void operator()(SDL_Surface* surface) const {
        SDL_FreeSurface(surface);
    }
};

struct TextureDeleter {
    void operator()(SDL_Texture* texture) const {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
    }
};

using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;

} // namespace sge::util
