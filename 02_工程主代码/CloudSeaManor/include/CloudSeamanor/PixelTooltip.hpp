#pragma once

#include "CloudSeamanor/PixelArtStyle.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

class PixelTooltip {
public:
    void SetContent(const std::string& title,
                    const std::string& quality,
                    const std::string& description,
                    const std::string& extra);
    void Show(const sf::Vector2f& mouse_pos);
    void Hide();
    void Update(float delta_seconds, const sf::Vector2f& mouse_pos);
    void Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const;

private:
    std::string m_title;
    std::string m_quality;
    std::string m_description;
    std::string m_extra;
    sf::Vector2f m_position{0.0f, 0.0f};
    float m_hover_timer = 0.0f;
    bool m_visible = false;
    bool m_pending_show = false;
};

}  // namespace CloudSeamanor::engine
