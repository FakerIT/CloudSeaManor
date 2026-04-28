#include "CloudSeamanor/PixelTeaGardenPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelTeaGardenPanel::PixelTeaGardenPanel()
    : PixelUiPanel({{360.0f, 140.0f}, {560.0f, 420.0f}}, "茶园管理", true) {
}

void PixelTeaGardenPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;

    m_font_renderer->DrawText(window,
                              data_.cloud_state_prefix + " " + data_.cloud_state_text + "  "
                                  + data_.spirit_bonus_prefix + std::to_string(data_.spirit_bonus) + "  "
                                  + data_.quality_bonus_prefix + std::to_string(data_.quality_bonus_percent) + "%",
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, data_.cloud_preview_prefix + " " + data_.cloud_preview_text, {x, y + row_h}, TextStyle::TopRightInfo());

    m_font_renderer->DrawText(window, data_.plots_title, {x, y + row_h * 2.2f}, TextStyle::PanelTitle());
    const std::size_t count = std::min<std::size_t>(3, data_.plots.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& plot = data_.plots[i];
        m_font_renderer->DrawText(window,
                                  plot.name + "   " + std::to_string(std::max(0, plot.progress_percent)) + "%  [" + plot.quality_text + "]",
                                  {x, y + row_h * (3.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }
    m_font_renderer->DrawText(window, data_.bush_countdown_title, {x, y + row_h * 6.0f}, TextStyle::PanelTitle());
    const std::size_t bush_count = std::min<std::size_t>(2, data_.bush_countdown_lines.size());
    for (std::size_t i = 0; i < bush_count; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.bush_countdown_lines[i],
                                  {x, y + row_h * (6.8f + static_cast<float>(i) * 0.9f)},
                                  TextStyle::Default());
    }

    sf::RectangleShape quality_hint({inner_rect.size.x - 24.0f, 24.0f});
    quality_hint.setPosition({x, y + row_h * 8.5f});
    quality_hint.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    quality_hint.setOutlineThickness(1.0f);
    quality_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(quality_hint);
    m_font_renderer->DrawText(window, data_.quality_hint_text, {x + 6.0f, y + row_h * 8.7f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 10.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
