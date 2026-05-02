#include "CloudSeamanor/engine/PixelNpcDetailPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelNpcDetailPanel::PixelNpcDetailPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "NPC 社交详情", true) {
}

void PixelNpcDetailPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    TextStyle title_style = TextStyle::PanelTitle();
    if (data_.legendary_style_unlocked) {
        title_style.fill_color = sf::Color(245, 211, 115, 255);
        sf::RectangleShape legend_border({inner_rect.size.x - 8.0f, inner_rect.size.y - 8.0f});
        legend_border.setPosition({inner_rect.position.x + 4.0f, inner_rect.position.y + 4.0f});
        legend_border.setFillColor(sf::Color(0, 0, 0, 0));
        legend_border.setOutlineThickness(2.0f);
        legend_border.setOutlineColor(sf::Color(245, 211, 115, 180));
        window.draw(legend_border);
    }
    m_font_renderer->DrawText(window, data_.name + "  |  " + data_.title_suffix, {x, y}, title_style);
    m_font_renderer->DrawText(window, data_.location_prefix + " " + data_.location, {x, y + row_h}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window,
                              data_.favor_prefix + " " + data_.cloud_stage_text
                                  + "（阶段可见，数值隐藏）",
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
