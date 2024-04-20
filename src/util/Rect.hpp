#pragma once

#include <glm/glm.hpp>

#include <cstdlib>

namespace sge::util {

struct Rect {
    double x, y;
    double w, h;

    constexpr double top() const {
        return this->y;
    }

    constexpr double bottom() const {
        return this->y + this->h;
    }

    constexpr double left() const {
        return this->x;
    }

    constexpr double right() const {
        return this->x + this->w;
    }

    bool collides(const Rect &other) const {
        return this->right() > other.left() && this->left() < other.right() &&
               this->bottom() > other.top() && this->top() < other.bottom();
    }
};

inline Rect RectCenteredAt(const glm::dvec2 &size, const glm::dvec2 &pos) {
    return Rect{
        pos.x - std::abs(size.x) / 2.0,
        pos.y - std::abs(size.y) / 2.0,
        std::abs(size.x),
        std::abs(size.y),
    };
}

struct RectF {
    float x, y;
    float w, h;

    constexpr float top() const {
        return this->y;
    }

    constexpr float bottom() const {
        return this->y + this->h;
    }

    constexpr float left() const {
        return this->x;
    }

    constexpr float right() const {
        return this->x + this->w;
    }

    bool collides(const RectF &other) const {
        return this->right() > other.left() && this->left() < other.right() &&
               this->bottom() > other.top() && this->top() < other.bottom();
    }
};

inline RectF RectCenteredAt(const glm::fvec2 &size, const glm::fvec2 &pos) {
    return RectF{
        pos.x - std::abs(size.x) / 2.0F,
        pos.y - std::abs(size.y) / 2.0F,
        std::abs(size.x),
        std::abs(size.y),
    };
}

} // namespace sge::util
