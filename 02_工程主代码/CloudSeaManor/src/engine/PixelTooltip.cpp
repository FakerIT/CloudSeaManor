#include "CloudSeamanor/engine/PixelTooltip.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>
#include <cstdint>

namespace CloudSeamanor::engine {

void PixelTooltip::SetContent(const std::string& title,
                              const std::string& quality,
                              const std::string& description,
                              const std::string& extra) {
    m_title = title;
    m_quality = quality;
    m_description = description;
    m_extra = extra;
}

void PixelTooltip::Show(const sf::Vector2f& mouse_pos) {
    m_position = {mouse_pos.x + 12.0f, mouse_pos.y + 12.0f};
    m_hover_timer = 0.0f;
    m_pending_show = true;
}

void PixelTooltip::Hide() {
    m_visible = false;
    m_pending_show = false;
    m_hover_timer = 0.0f;
}

void PixelTooltip::Update(float delta_seconds, const sf::Vector2f& mouse_pos) {
    m_position = {mouse_pos.x + 12.0f, mouse_pos.y + 12.0f};
    if (!m_pending_show) return;
    m_hover_timer += delta_seconds;
    if (m_hover_timer >= 0.3f) {
        m_visible = true;
    }
}

void PixelTooltip::Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const {
    if (!m_visible) return;

    constexpr float kMaxWidth = 200.0f;
    constexpr float kPadding = 8.0f;
    constexpr float kTitleFont = 16.0f;
    constexpr float kBodyFont = 12.0f;

    // Layout: title(16) + quality(12) + separator + description + extra
    // Note: PixelFontRenderer provides wrapped text drawing; height here is a safe bound.
    const float width = kMaxWidth;
    const float height = 132.0f;

    sf::VertexArray panel(sf::PrimitiveType::Triangles);
    const sf::Color fill = sf::Color(ColorPalette::Cream.r, ColorPalette::Cream.g, ColorPalette::Cream.b, 250);
    const sf::Color border = ColorPalette::BrownOutline;

    auto AddQuad = [&](const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3, const sf::Color& color) {
        panel.append(sf::Vertex(p0, color));
        panel.append(sf::Vertex(p1, color));
        panel.append(sf::Vertex(p2, color));
        panel.append(sf::Vertex(p0, color));
        panel.append(sf::Vertex(p2, color));
        panel.append(sf::Vertex(p3, color));
    };

    AddQuad({m_position.x, m_position.y}, {m_position.x + width, m_position.y},
            {m_position.x + width, m_position.y + height}, {m_position.x, m_position.y + height}, fill);
    AddQuad({m_position.x, m_position.y}, {m_position.x + width, m_position.y},
            {m_position.x + width, m_position.y + 1.0f}, {m_position.x, m_position.y + 1.0f}, border);
    AddQuad({m_position.x, m_position.y + height - 1.0f}, {m_position.x + width, m_position.y + height - 1.0f},
            {m_position.x + width, m_position.y + height}, {m_position.x, m_position.y + height}, border);
    AddQuad({m_position.x, m_position.y}, {m_position.x + 1.0f, m_position.y},
            {m_position.x + 1.0f, m_position.y + height}, {m_position.x, m_position.y + height}, border);
    AddQuad({m_position.x + width - 1.0f, m_position.y}, {m_position.x + width, m_position.y},
            {m_position.x + width, m_position.y + height}, {m_position.x + width - 1.0f, m_position.y + height}, border);

    window.draw(panel, sf::RenderStates::Default);

    TextStyle title_style = TextStyle::PanelTitle();
    title_style.character_size = static_cast<unsigned int>(kTitleFont);
    title_style.bold = true;
    title_style.fill_color = ColorPalette::DeepBrown;

    TextStyle body_style = TextStyle::Default();
    body_style.character_size = static_cast<unsigned int>(kBodyFont);
    body_style.fill_color = ColorPalette::TextBrown;

    TextStyle extra_style = TextStyle::CoinText();
    extra_style.character_size = static_cast<unsigned int>(kBodyFont);

    const float x = m_position.x + kPadding;
    float y = m_position.y + 6.0f;
    font_renderer.DrawText(window, m_title, {x, y}, title_style);
    y += 20.0f;

    // Quality label (colored)
    TextStyle quality_style = body_style;
    quality_style.bold = true;
    if (m_quality.find("圣") != std::string::npos || m_quality.find("传说") != std::string::npos) {
        quality_style.fill_color = ColorPalette::CoinGold;
    } else if (m_quality.find("珍") != std::string::npos || m_quality.find("稀") != std::string::npos) {
        quality_style.fill_color = ColorPalette::Season::SummerBlue;
    } else if (m_quality.find("优") != std::string::npos) {
        quality_style.fill_color = ColorPalette::ActiveGreen;
    } else {
        quality_style.fill_color = ColorPalette::LightGray;
    }
    font_renderer.DrawText(window, m_quality, {x, y}, quality_style);
    y += 18.0f;

    // Separator
    TextStyle sep_style = body_style;
    sep_style.fill_color = ColorPalette::LightBrown;
    font_renderer.DrawText(window, "---", {x, y}, sep_style);
    y += 16.0f;

    // Description (wrapped)
    font_renderer.DrawWrappedText(window, m_description, {x, y}, width - kPadding * 2.0f, body_style, 1.2f);
    y += 44.0f;

    // Extra line (price/effect)
    font_renderer.DrawText(window, m_extra, {x, std::min(m_position.y + height - 22.0f, y)}, extra_style);
}

}  // namespace CloudSeamanor::engine
