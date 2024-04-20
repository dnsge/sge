#include "render/RenderQueue.hpp"

#include "Renderer.hpp"

#include <algorithm>
#include <utility>
#include <variant>

namespace sge::render {

void RenderQueue::render(const glm::vec2 &cameraPos, float zoom) {
    this->renderImage(cameraPos, zoom);
    this->renderUi();
    this->renderText();
    this->renderPixel();
}

void RenderQueue::enqueueImage(DrawImageArgs args) {
    this->imageQueue_.emplace_back(args, 0);
}

void RenderQueue::enqueueImageEx(DrawImageExArgs args, int sortOrder) {
    this->imageQueue_.emplace_back(args, sortOrder);
}

void RenderQueue::enqueueUi(DrawUiArgs args) {
    this->uiQueue_.emplace_back(args, 0);
}

void RenderQueue::enqueueUiEx(DrawUiExArgs args, int sortOrder) {
    this->uiQueue_.emplace_back(args, sortOrder);
}

void RenderQueue::enqueueText(DrawTextArgs args) {
    this->textQueue_.emplace_back(std::move(args));
}

void RenderQueue::enqueuePixel(DrawPixelArgs args) {
    this->pixelQueue_.emplace_back(args);
}

void RenderQueue::renderImage(const glm::vec2 &cameraPos, float zoom) {
    // Sort according to sort order field
    std::stable_sort(
        this->imageQueue_.begin(), this->imageQueue_.end(), [](const auto &a, const auto &b) {
            return a.second < b.second;
        });

    // Render
    for (auto &image : this->imageQueue_) {
        if (const auto* req = std::get_if<DrawImageArgs>(&image.first)) {
            Renderer::renderImage(req->image, req->x, req->y, cameraPos, zoom);
        } else if (const auto* req = std::get_if<DrawImageExArgs>(&image.first)) {
            Renderer::renderImageEx(*req, cameraPos, zoom);
        }
    }
    this->imageQueue_.clear();
}

void RenderQueue::renderUi() {
    // Sort according to sort order field
    std::stable_sort(
        this->uiQueue_.begin(), this->uiQueue_.end(), [](const auto &a, const auto &b) {
            return a.second < b.second;
        });

    // Render
    for (auto &ui : this->uiQueue_) {
        if (const auto* req = std::get_if<DrawUiArgs>(&ui.first)) {
            Renderer::renderUiImage(req->image, req->location);
        } else if (const auto* req = std::get_if<DrawUiExArgs>(&ui.first)) {
            Renderer::renderUiImageEx(req->image, req->location, req->color);
        }
    }
    this->uiQueue_.clear();
}

void RenderQueue::renderText() {
    for (auto &text : this->textQueue_) {
        Renderer::renderText(text.text, text.location.x, text.location.y);
    }
    this->textQueue_.clear();
}

void RenderQueue::renderPixel() {
    for (auto &pixel : this->pixelQueue_) {
        Renderer::renderPixel(pixel.location, pixel.color);
    }
    this->pixelQueue_.clear();
}

} // namespace sge::render
