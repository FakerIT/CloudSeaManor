#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace CloudSeamanor::engine::uivx {

[[nodiscard]] inline sf::Color WithAlpha(const sf::Color& color, float alpha = 1.0f) {
    sf::Color out = color;
    out.a = static_cast<std::uint8_t>(
        std::clamp(static_cast<float>(color.a) * alpha, 0.0f, 255.0f));
    return out;
}

[[nodiscard]] inline sf::Vector2f SnapToPixel(const sf::Vector2f& p) {
    return {std::round(p.x), std::round(p.y)};
}

inline void AddQuad(sf::VertexArray& va,
                    const sf::Vector2f& p0,
                    const sf::Vector2f& p1,
                    const sf::Vector2f& p2,
                    const sf::Vector2f& p3,
                    const sf::Color& color,
                    float alpha = 1.0f,
                    bool snap = true) {
    const sf::Color c = WithAlpha(color, alpha);
    const sf::Vector2f s0 = snap ? SnapToPixel(p0) : p0;
    const sf::Vector2f s1 = snap ? SnapToPixel(p1) : p1;
    const sf::Vector2f s2 = snap ? SnapToPixel(p2) : p2;
    const sf::Vector2f s3 = snap ? SnapToPixel(p3) : p3;

    va.append(sf::Vertex(s0, c));
    va.append(sf::Vertex(s1, c));
    va.append(sf::Vertex(s2, c));
    va.append(sf::Vertex(s0, c));
    va.append(sf::Vertex(s2, c));
    va.append(sf::Vertex(s3, c));
}

}  // namespace CloudSeamanor::engine::uivx
