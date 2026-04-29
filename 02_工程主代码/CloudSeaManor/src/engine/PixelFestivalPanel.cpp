#include "CloudSeamanor/engine/PixelFestivalPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::engine {

PixelFestivalPanel::PixelFestivalPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "节日活动", true) {
}

void PixelFestivalPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window,
                              data_.active_name + "  倒计时: " + std::to_string(std::max(0, data_.countdown_days)) + " 天",
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window,
                              "活动进度: " + std::to_string(std::max(0, data_.completed_events)) + "/" +
                                  std::to_string(std::max(1, data_.total_events)),
                              {x, y + row_h},
                              TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, data_.reward_text, {x, y + row_h * 2.1f}, TextStyle::Default());
    std::string upcoming_text = data_.upcoming_prefix;
    if (data_.upcoming.empty()) {
        upcoming_text += data_.upcoming_empty_text;
    } else {
        const std::size_t count = std::min<std::size_t>(3, data_.upcoming.size());
        std::ostringstream suffix;
        for (std::size_t i = 0; i < count; ++i) {
            if (i > 0) {
                suffix << "  ";
            }
            suffix << data_.upcoming[i];
        }
        upcoming_text += suffix.str();
    }
    m_font_renderer->DrawText(window, upcoming_text, {x, y + row_h * 3.1f}, TextStyle::Default());
    m_font_renderer->DrawText(window, data_.participation_text, {x, y + row_h * 4.1f}, TextStyle::Default());

    sf::RectangleShape mode_hint({inner_rect.size.x - 24.0f, 24.0f});
    mode_hint.setPosition({x, y + row_h * 5.4f});
    mode_hint.setFillColor(ColorPalette::HighlightYellow);
    mode_hint.setOutlineThickness(1.0f);
    mode_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(mode_hint);
    m_font_renderer->DrawText(window, data_.selected_participation_text, {x + 6.0f, y + row_h * 5.65f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 7.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
