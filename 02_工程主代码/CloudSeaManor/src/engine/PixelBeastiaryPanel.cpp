#include "CloudSeamanor/engine/PixelBeastiaryPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelBeastiaryPanel::PixelBeastiaryPanel()
    : PixelUiPanel({{340.0f, 120.0f}, {600.0f, 480.0f}}, "灵兽图鉴", true) {
}

void PixelBeastiaryPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window, data_.filter_text, {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window,
                              data_.progress_prefix + " " + std::to_string(data_.discovered_count) + "/" +
                                  std::to_string(std::max(1, data_.total_count)),
                              {x, y + row_h},
                              TextStyle::PanelTitle());
    const std::size_t discovered_count = std::min<std::size_t>(2, data_.discovered_lines.size());
    for (std::size_t i = 0; i < discovered_count; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.discovered_lines[i],
                                  {x, y + row_h * (2.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }
    const std::size_t undiscovered_count = std::min<std::size_t>(2, data_.undiscovered_lines.size());
    for (std::size_t i = 0; i < undiscovered_count; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.undiscovered_lines[i],
                                  {x, y + row_h * (4.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }

    sf::RectangleShape detail_panel({inner_rect.size.x - 24.0f, 24.0f});
    detail_panel.setPosition({x, y + row_h * 6.5f});
    detail_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    detail_panel.setOutlineThickness(1.0f);
    detail_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(detail_panel);
    m_font_renderer->DrawText(window, data_.selected_detail, {x + 6.0f, y + row_h * 6.75f}, TextStyle::HotkeyHint());
}

}  // namespace CloudSeamanor::engine
