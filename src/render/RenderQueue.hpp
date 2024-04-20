#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "render/Text.hpp"
#include "resources/Resources.hpp"

#include <utility>
#include <variant>
#include <vector>

namespace sge::render {

struct DrawTextArgs {
    render::Text text;
    SDL_Point location;
};

struct DrawUiArgs {
    resources::Image image;
    SDL_Point location;
};

struct DrawUiExArgs {
    resources::Image image;
    SDL_Point location;
    SDL_Color color;
};

struct DrawImageArgs {
    resources::Image image;
    float x, y;
};

struct DrawImageExArgs {
    resources::Image image;
    float x, y;
    int rotation;
    float scaleX, scaleY;
    float pivotX, pivotY;
    SDL_Color color;
};

struct DrawPixelArgs {
    SDL_Point location;
    SDL_Color color;
};

class RenderQueue {
public:
    RenderQueue() = default;

    void render(const glm::vec2 &cameraPos, float zoom);

    void enqueueImage(DrawImageArgs args);
    void enqueueImageEx(DrawImageExArgs args, int sortOrder);
    void enqueueUi(DrawUiArgs args);
    void enqueueUiEx(DrawUiExArgs args, int sortOrder);
    void enqueueText(DrawTextArgs args);
    void enqueuePixel(DrawPixelArgs args);

private:
    void renderImage(const glm::vec2 &cameraPos, float zoom);
    void renderUi();
    void renderText();
    void renderPixel();

    using UiRequest = std::pair<std::variant<DrawUiArgs, DrawUiExArgs>, int>;
    using ImageRequest = std::pair<std::variant<DrawImageArgs, DrawImageExArgs>, int>;

    std::vector<DrawTextArgs> textQueue_;
    std::vector<UiRequest> uiQueue_;
    std::vector<ImageRequest> imageQueue_;
    std::vector<DrawPixelArgs> pixelQueue_;
};

} // namespace sge::render
