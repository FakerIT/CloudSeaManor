#include "CloudSeamanor/PixelMailPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelMailPanel::PixelMailPanel()
    : PixelUiPanel({{400.0f, 160.0f}, {480.0f, 400.0f}}, "邮件与订单", true) {
}

void PixelMailPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window, data_.list_title_text, {x, y}, TextStyle::PanelTitle());
    const std::size_t count = std::min<std::size_t>(3, data_.entries.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& entry = data_.entries[i];
        const std::string line = entry.sender + "  |  " + entry.subject + "  |  " + entry.time_text;
        m_font_renderer->DrawText(window, line, {x, y + row_h * (1.0f + static_cast<float>(i))}, TextStyle::Default());
    }
    const std::string detail = data_.detail_text.empty() ? data_.empty_detail_text : data_.detail_text;
    m_font_renderer->DrawText(window, detail, {x, y + row_h * 4.2f}, TextStyle::TopRightInfo());

    sf::RectangleShape unread_hint({inner_rect.size.x - 24.0f, 24.0f});
    unread_hint.setPosition({x, y + row_h * 5.4f});
    unread_hint.setFillColor(ColorPalette::WarningPink);
    unread_hint.setOutlineThickness(1.0f);
    unread_hint.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(unread_hint);
    m_font_renderer->DrawText(
        window,
        data_.unread_prefix_text + " " + std::to_string(std::max(0, data_.unread_count)) + " " + data_.unread_suffix_text,
        {x + 6.0f, y + row_h * 5.65f},
        TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 7.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
