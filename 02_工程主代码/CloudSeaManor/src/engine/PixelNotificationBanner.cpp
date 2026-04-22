#include "CloudSeamanor/PixelNotificationBanner.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <cstdint>

namespace CloudSeamanor::engine {

void PixelNotificationBanner::Push(const std::string& message) {
    if (m_queue.size() >= 3) {
        m_queue.pop_front();
    }
    m_queue.push_back({message, 0.0f});
}

void PixelNotificationBanner::Update(float delta_seconds) {
    for (auto& item : m_queue) {
        item.timer += delta_seconds;
    }
    while (!m_queue.empty() && m_queue.front().timer > 3.6f) {
        m_queue.pop_front();
    }
}

void PixelNotificationBanner::Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const {
    constexpr float kWidth = 600.0f;
    constexpr float kHeight = 36.0f;
    constexpr float kTop = 16.0f;
    constexpr float kSpacing = 4.0f;

    const float window_width = static_cast<float>(window.getSize().x);
    const float x = (window_width - kWidth) * 0.5f;

    for (std::size_t i = 0; i < m_queue.size(); ++i) {
        const auto& item = m_queue[i];
        const float y = kTop + static_cast<float>(i) * (kHeight + kSpacing);
        const float t = item.timer;
        float alpha = 1.0f;
        if (t < 0.3f) alpha = t / 0.3f;
        if (t > 3.3f) alpha = (3.6f - t) / 0.3f;
        if (alpha <= 0.0f) continue;

        const sf::Color fill = sf::Color(ColorPalette::Cream.r, ColorPalette::Cream.g, ColorPalette::Cream.b, static_cast<std::uint8_t>(220.0f * alpha));
        const sf::Color top_strip = sf::Color(ColorPalette::LightBrown.r, ColorPalette::LightBrown.g, ColorPalette::LightBrown.b, static_cast<std::uint8_t>(255.0f * alpha));

        sf::VertexArray panel(sf::PrimitiveType::Triangles);
        auto AddQuad = [&](const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3, const sf::Color& color) {
            panel.append(sf::Vertex(p0, color));
            panel.append(sf::Vertex(p1, color));
            panel.append(sf::Vertex(p2, color));
            panel.append(sf::Vertex(p0, color));
            panel.append(sf::Vertex(p2, color));
            panel.append(sf::Vertex(p3, color));
        };
        AddQuad({x, y}, {x + kWidth, y}, {x + kWidth, y + kHeight}, {x, y + kHeight}, fill);
        AddQuad({x, y}, {x + kWidth, y}, {x + kWidth, y + 2.0f}, {x, y + 2.0f}, top_strip);
        window.draw(panel, sf::RenderStates::Default);

        font_renderer.DrawText(window, item.text, {x + 12.0f, y + 10.0f}, TextStyle::Default());
    }
}

}  // namespace CloudSeamanor::engine
