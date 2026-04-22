#include "CloudSeamanor/PixelNpcDetailPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelNpcDetailPanel::PixelNpcDetailPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "NPC 社交详情", true) {
}

void PixelNpcDetailPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window, data_.name + "  |  " + data_.title_suffix, {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, data_.location_prefix + " " + data_.location, {x, y + row_h}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window,
                              data_.favor_prefix + " " + std::to_string(data_.heart_level) + "/10  (favor " + std::to_string(data_.favor) + ")",
                              {x, y + row_h * 2.1f},
                              TextStyle::Default());
    m_font_renderer->DrawText(window, data_.heart_event_text, {x, y + row_h * 3.1f}, TextStyle::Default());
    m_font_renderer->DrawText(window,
                              "今日互动: " + std::string(data_.talked_today ? data_.talked_done_text : data_.talked_todo_text) +
                                  "  送礼状态: " + (data_.gifted_today ? data_.gifted_done_text : data_.gifted_todo_text),
                              {x, y + row_h * 4.1f},
                              TextStyle::Default());

    sf::RectangleShape event_hint({inner_rect.size.x - 24.0f, 24.0f});
    event_hint.setPosition({x, y + row_h * 5.4f});
    event_hint.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    event_hint.setOutlineThickness(1.0f);
    event_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(event_hint);
    m_font_renderer->DrawText(window, data_.event_hint_text, {x + 6.0f, y + row_h * 5.65f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 7.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
