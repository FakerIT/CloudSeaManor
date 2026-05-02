#pragma once

#include <optional>

namespace CloudSeamanor::domain {

struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;

    [[nodiscard]] constexpr Vec2f operator+(const Vec2f& other) const noexcept {
        return Vec2f{x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr Vec2f operator-(const Vec2f& other) const noexcept {
        return Vec2f{x - other.x, y - other.y};
    }

    [[nodiscard]] constexpr Vec2f operator*(float scalar) const noexcept {
        return Vec2f{x * scalar, y * scalar};
    }
};

struct RectF {
    Vec2f position{};
    Vec2f size{};

    [[nodiscard]] constexpr float left() const noexcept { return position.x; }
    [[nodiscard]] constexpr float top() const noexcept { return position.y; }
    [[nodiscard]] constexpr float right() const noexcept { return position.x + size.x; }
    [[nodiscard]] constexpr float bottom() const noexcept { return position.y + size.y; }
};

[[nodiscard]] constexpr RectF MakeRect(Vec2f pos, Vec2f size) noexcept {
    return RectF{pos, size};
}

[[nodiscard]] constexpr bool Intersects(const RectF& a, const RectF& b) noexcept {
    // Note: treat edge-touch as non-intersection (matches typical AABB overlap usage).
    return (a.left() < b.right())
        && (a.right() > b.left())
        && (a.top() < b.bottom())
        && (a.bottom() > b.top());
}

[[nodiscard]] constexpr std::optional<RectF> Intersection(const RectF& a, const RectF& b) noexcept {
    if (!Intersects(a, b)) {
        return std::nullopt;
    }
    const float nx = (a.left() > b.left()) ? a.left() : b.left();
    const float ny = (a.top() > b.top()) ? a.top() : b.top();
    const float nr = (a.right() < b.right()) ? a.right() : b.right();
    const float nb = (a.bottom() < b.bottom()) ? a.bottom() : b.bottom();
    return RectF{Vec2f{nx, ny}, Vec2f{nr - nx, nb - ny}};
}

}  // namespace CloudSeamanor::domain

