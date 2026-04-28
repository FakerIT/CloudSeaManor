#include "CloudSeamanor/PixelSpiritBeastPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelSpiritBeastPanel::PixelSpiritBeastPanel()
    : PixelUiPanel({{320.0f, 100.0f}, {640.0f, 520.0f}}, "灵兽小屋", true) {
}

void PixelSpiritBeastPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window, data_.category_text, {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, data_.cards_title, {x, y + row_h}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window,
                              data_.beast_name + "  | " + data_.state_text + " | 羁绊: " + std::to_string(data_.favor),
                              {x, y + row_h * 2.1f},
                              TextStyle::Default());
    if (data_.dispatched && data_.dispatch_remaining_seconds > 0) {
        m_font_renderer->DrawText(
            window,
            "派遣剩余: " + std::to_string(data_.dispatch_remaining_seconds) + "s（完成后将投递到邮件）",
            {x, y + row_h * 2.95f},
            TextStyle::HotkeyHint());
    }
    m_font_renderer->DrawText(window,
                              "特质: " + data_.trait + "  | 派遣: " +
                                  (data_.dispatched ? data_.dispatch_in_progress_text : data_.dispatch_idle_text),
                              {x, y + row_h * 3.75f},
                              TextStyle::Default());
    m_font_renderer->DrawText(window, data_.influence_hint_text, {x, y + row_h * 4.1f}, TextStyle::Default());

    sf::RectangleShape recruit_hint({inner_rect.size.x - 24.0f, 24.0f});
    recruit_hint.setPosition({x, y + row_h * 5.4f});
    recruit_hint.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    recruit_hint.setOutlineThickness(1.0f);
    recruit_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(recruit_hint);
    m_font_renderer->DrawText(window, data_.recruit_hint_text, {x + 6.0f, y + row_h * 5.65f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 7.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
