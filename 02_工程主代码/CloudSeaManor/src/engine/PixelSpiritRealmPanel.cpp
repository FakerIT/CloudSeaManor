#include "CloudSeamanor/PixelSpiritRealmPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelSpiritRealmPanel::PixelSpiritRealmPanel()
    : PixelUiPanel({{340.0f, 120.0f}, {600.0f, 480.0f}}, "灵界探索", true) {
}

void PixelSpiritRealmPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(
        window,
        data_.mode_line_prefix + " [" + data_.mode_text + "✓] " + data_.mode_options_text,
        {x, y},
        TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window,
                              data_.remaining_line_prefix + " " + std::to_string(data_.remaining_count) + "/" + std::to_string(data_.max_count) +
                                  "  " + data_.drop_bonus_prefix + std::to_string(data_.drop_bonus_percent) + "%",
                              {x, y + row_h},
                              TextStyle::Default());
    m_font_renderer->DrawText(window, data_.regions_title, {x, y + row_h * 2.2f}, TextStyle::PanelTitle());
    const std::string active_state = data_.in_spirit_realm
        ? data_.active_state_in_realm_text
        : data_.active_state_unlocked_text;
    if (data_.region_lines.empty()) {
        m_font_renderer->DrawText(
            window,
            data_.default_region_line_1 + data_.active_state_suffix + active_state,
            {x, y + row_h * 3.2f},
            TextStyle::Default());
        m_font_renderer->DrawText(window, data_.default_region_line_2, {x, y + row_h * 4.2f}, TextStyle::Default());
        m_font_renderer->DrawText(window, data_.default_region_line_3, {x, y + row_h * 5.2f}, TextStyle::Default());
    } else {
        const std::size_t count = std::min<std::size_t>(3, data_.region_lines.size());
        for (std::size_t i = 0; i < count; ++i) {
            std::string line = data_.region_lines[i];
            if (i == 0) {
                line += data_.active_state_suffix + active_state;
            }
            m_font_renderer->DrawText(window, line, {x, y + row_h * (3.2f + static_cast<float>(i))}, TextStyle::Default());
        }
    }

    sf::RectangleShape lock_hint({inner_rect.size.x - 24.0f, 24.0f});
    lock_hint.setPosition({x, y + row_h * 6.5f});
    lock_hint.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    lock_hint.setOutlineThickness(1.0f);
    lock_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(lock_hint);
    m_font_renderer->DrawText(window, data_.lock_hint_text, {x + 6.0f, y + row_h * 6.75f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
