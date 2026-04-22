#pragma once

#include "CloudSeamanor/MathTypes.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace CloudSeamanor::adapter {

[[nodiscard]] constexpr sf::Vector2f ToSf(const domain::Vec2f& v) noexcept {
    return sf::Vector2f(v.x, v.y);
}

[[nodiscard]] constexpr domain::Vec2f ToDomain(const sf::Vector2f& v) noexcept {
    return domain::Vec2f{v.x, v.y};
}

[[nodiscard]] constexpr sf::FloatRect ToSf(const domain::RectF& r) noexcept {
    return sf::FloatRect(sf::Vector2f(r.position.x, r.position.y), sf::Vector2f(r.size.x, r.size.y));
}

[[nodiscard]] constexpr domain::RectF ToDomain(const sf::FloatRect& r) noexcept {
    return domain::RectF{domain::Vec2f{r.position.x, r.position.y}, domain::Vec2f{r.size.x, r.size.y}};
}

[[nodiscard]] constexpr sf::Color PackedRgbaToColor(std::uint32_t rgba) noexcept {
    return sf::Color(
        static_cast<std::uint8_t>((rgba >> 24) & 0xFFu),
        static_cast<std::uint8_t>((rgba >> 16) & 0xFFu),
        static_cast<std::uint8_t>((rgba >> 8) & 0xFFu),
        static_cast<std::uint8_t>(rgba & 0xFFu));
}

}  // namespace CloudSeamanor::adapter
