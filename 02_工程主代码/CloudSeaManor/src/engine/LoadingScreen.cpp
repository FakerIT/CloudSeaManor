#include "CloudSeamanor/engine/LoadingScreen.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <cmath>

namespace CloudSeamanor::engine {

void LoadingScreen::Update(float delta_seconds) {
    if (!m_visible) return;
    m_timer += delta_seconds;
}

void LoadingScreen::Render(sf::RenderWindow& window, PixelFontRenderer* font_renderer) const {
    if (!m_visible) return;
    const sf::Vector2u size = window.getSize();

    sf::RectangleShape mask;
    mask.setSize({static_cast<float>(size.x), static_cast<float>(size.y)});
    mask.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(mask);

    const float cx = static_cast<float>(size.x) * 0.5f;
    const float cy = static_cast<float>(size.y) * 0.5f + 18.0f;
    const float bar_w = 280.0f;
    const float bar_h = 10.0f;
    const float pulse = std::fmod(m_timer * 120.0f, bar_w + 56.0f) - 28.0f;

    sf::RectangleShape bar_bg;
    bar_bg.setPosition({cx - bar_w * 0.5f, cy});
    bar_bg.setSize({bar_w, bar_h});
    bar_bg.setFillColor(sf::Color(32, 32, 32, 220));
    bar_bg.setOutlineThickness(1.0f);
    bar_bg.setOutlineColor(sf::Color(186, 140, 96, 230));
    window.draw(bar_bg);

    sf::RectangleShape pulse_block;
    pulse_block.setPosition({cx - bar_w * 0.5f + pulse, cy + 1.0f});
    pulse_block.setSize({56.0f, bar_h - 2.0f});
    pulse_block.setFillColor(sf::Color(246, 214, 146, 220));
    window.draw(pulse_block);

    if (font_renderer != nullptr && font_renderer->IsLoaded()) {
        const float dots = std::fmod(m_timer, 1.0f) * 3.0f;
        std::string suffix(static_cast<std::size_t>(dots), '.');
        font_renderer->DrawText(window,
                                m_stage_text + suffix,
                                {static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.5f - 10.0f},
                                TextStyle::PanelTitle(),
                                TextAlignment::Center);
    }
}

}  // namespace CloudSeamanor::engine
