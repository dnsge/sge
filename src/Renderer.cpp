#include "Renderer.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include "Constants.hpp"
#include "gea/AudioHelper.h"
#include "gea/Helper.h"
#include "render/RenderQueue.hpp"
#include "render/Text.hpp"
#include "resources/Configs.hpp"
#include "resources/Resources.hpp"
#include "util/Rect.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace sge {

namespace {

resources::RenderingConfig Config{};
int WindowHalfWidth = 0;
int WindowHalfHeight = 0;

SDL_Window* GlobalWindow = nullptr;
SDL_Renderer* GlobalRenderer = nullptr;

std::string GlobalFontName;

} // namespace

void Renderer::initialize(const std::string &windowTitle, resources::RenderingConfig config,
                          const std::string &fontName) {
    assert(GlobalWindow == nullptr);
    assert(GlobalRenderer == nullptr);

    Config = config;
    WindowHalfWidth = Config.x_resolution / 2;
    WindowHalfHeight = Config.y_resolution / 2;

    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL initialization failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    // Construct SDL window
    GlobalWindow = Helper::SDL_CreateWindow498(windowTitle.c_str(),
                                               SDL_WINDOWPOS_CENTERED,
                                               SDL_WINDOWPOS_CENTERED,
                                               config.x_resolution,
                                               config.y_resolution,
                                               SDL_WINDOW_SHOWN);
    assert(GlobalWindow != nullptr);

    // Construct SDL renderer
    GlobalRenderer = Helper::SDL_CreateRenderer498(
        GlobalWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    assert(GlobalRenderer != nullptr);

    // Initialize TTF
    if (TTF_Init() < 0) {
        std::cout << "SDL TTF initialization failed: " << TTF_GetError() << std::endl;
        std::exit(1);
    }

    // Initialize Audio
    if (AudioHelper::Mix_OpenAudio498(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        std::cout << "SDL Mix initialization failed: " << Mix_GetError() << std::endl;
        std::exit(1);
    }
    AudioHelper::Mix_AllocateChannels498(AudioChannelCount);

    // Set global font name
    GlobalFontName = fontName;
}

SDL_Window* Renderer::globalWindow() {
    return GlobalWindow;
}

SDL_Renderer* Renderer::globalRenderer() {
    return GlobalRenderer;
}

const std::string &Renderer::globalFontName() {
    return GlobalFontName;
}

int Renderer::windowWidth() {
    return Config.x_resolution;
}

int Renderer::windowHeight() {
    return Config.y_resolution;
}

void Renderer::renderClear() {
    SDL_SetRenderDrawColor(GlobalRenderer,
                           Config.clear_color_r,
                           Config.clear_color_g,
                           Config.clear_color_b,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(GlobalRenderer);
}

void Renderer::renderUiImage(const resources::Image &img, const SDL_Point &pos) {
    auto dest = SDL_Rect{pos.x, pos.y, img.width(), img.height()};
    SDL_RenderCopy(GlobalRenderer, img.texture_, nullptr, &dest);
}

void Renderer::renderUiImageEx(const resources::Image &img, const SDL_Point &pos,
                               const SDL_Color &color) {
    auto dest = SDL_Rect{pos.x, pos.y, img.width(), img.height()};
    SDL_SetTextureColorMod(img.texture_, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(img.texture_, color.a);
    SDL_RenderCopy(GlobalRenderer, img.texture_, nullptr, &dest);
    SDL_SetTextureColorMod(img.texture_, 255, 255, 255);
    SDL_SetTextureAlphaMod(img.texture_, 255);
}

void Renderer::renderImage(const resources::Image &img, float x, float y,
                           const glm::vec2 &cameraPos, float zoom) {
    // Compute adjustment factors for window centering and camera position
    float xZoomAdjustment = WindowHalfWidth / zoom;
    float yZoomAdjustment = WindowHalfHeight / zoom;
    float cameraX = cameraPos.x * PixelsPerMeter;
    float cameraY = cameraPos.y * PixelsPerMeter;

    auto viewRect = util::RectF{
        x * PixelsPerMeter,
        y * PixelsPerMeter,
        static_cast<float>(img.width()),
        static_cast<float>(img.height()),
    };
    auto pivotPoint = SDL_Point{
        static_cast<int>(0.5F * img.width()),
        static_cast<int>(0.5F * img.height()),
    };

    auto destX = viewRect.x - pivotPoint.x - cameraX + xZoomAdjustment;
    auto destY = viewRect.y - pivotPoint.y - cameraY + yZoomAdjustment;

    auto destRect = SDL_Rect{
        static_cast<int>(destX),
        static_cast<int>(destY),
        static_cast<int>(viewRect.w),
        static_cast<int>(viewRect.h),
    };

    SDL_RenderSetScale(GlobalRenderer, zoom, zoom);
    Helper::SDL_RenderCopyEx498(
        0, "", GlobalRenderer, img.texture_, nullptr, &destRect, 0.0F, &pivotPoint, SDL_FLIP_NONE);
    SDL_RenderSetScale(GlobalRenderer, 1.0F, 1.0F);
}

void Renderer::renderImageEx(const render::DrawImageExArgs &args, const glm::fvec2 &cameraPos,
                             float zoom) {
    // Compute adjustment factors for window centering and camera position
    float xZoomAdjustment = WindowHalfWidth / zoom;
    float yZoomAdjustment = WindowHalfHeight / zoom;

    glm::fvec2 renderPosition = glm::fvec2{args.x, args.y} - cameraPos;

    auto viewRect = util::RectF{
        renderPosition.x * PixelsPerMeter,
        renderPosition.y * PixelsPerMeter,
        std::abs(args.image.width() * args.scaleX),
        std::abs(args.image.height() * args.scaleY),
    };
    auto pivotPoint = SDL_Point{
        static_cast<int>(args.pivotX * args.image.width() * args.scaleX),
        static_cast<int>(args.pivotY * args.image.height() * args.scaleY),
    };

    auto destX = viewRect.x - pivotPoint.x + xZoomAdjustment;
    auto destY = viewRect.y - pivotPoint.y + yZoomAdjustment;

    auto destRect = SDL_Rect{
        static_cast<int>(destX),
        static_cast<int>(destY),
        static_cast<int>(viewRect.w),
        static_cast<int>(viewRect.h),
    };

    // Apply flip as appropriate from scale
    int flipFlags = SDL_FLIP_NONE;
    if (args.scaleX < 0) {
        flipFlags |= SDL_FLIP_HORIZONTAL;
    }
    if (args.scaleY < 0) {
        flipFlags |= SDL_FLIP_VERTICAL;
    }

    SDL_RenderSetScale(GlobalRenderer, zoom, zoom);
    SDL_SetTextureColorMod(args.image.texture_, args.color.r, args.color.g, args.color.b);
    SDL_SetTextureAlphaMod(args.image.texture_, args.color.a);
    Helper::SDL_RenderCopyEx498(0,
                                "",
                                GlobalRenderer,
                                args.image.texture_,
                                nullptr,
                                &destRect,
                                static_cast<double>(args.rotation),
                                &pivotPoint,
                                static_cast<SDL_RendererFlip>(flipFlags));
    SDL_RenderSetScale(GlobalRenderer, 1.0F, 1.0F);
    SDL_SetTextureColorMod(args.image.texture_, 255, 255, 255);
    SDL_SetTextureAlphaMod(args.image.texture_, 255);
}

void Renderer::renderText(const render::Text &text, int x, int y) {
    if (!text.validFont()) {
        std::cout << "error: text render failed. No font configured";
        std::exit(0);
    }

    auto destRect = SDL_Rect{x, y, text.width(), text.height()};
    SDL_RenderCopy(GlobalRenderer, text.texture_.get(), nullptr, &destRect);
}

void Renderer::renderPixel(const SDL_Point &location, const SDL_Color &color) {
    SDL_SetRenderDrawBlendMode(GlobalRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(GlobalRenderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(GlobalRenderer, location.x, location.y);
    SDL_SetRenderDrawBlendMode(GlobalRenderer, SDL_BLENDMODE_NONE);
}

void Renderer::present() {
    Helper::SDL_RenderPresent498(GlobalRenderer);
}

} // namespace sge
