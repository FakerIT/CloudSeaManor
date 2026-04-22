#include "CloudSeamanor/PixelShopPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>
#include <string>

namespace CloudSeamanor::engine {

PixelShopPanel::PixelShopPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "商店", true) {
}

void PixelShopPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window, data_.shop_name, {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, data_.items_header_text, {x, y + row_h}, TextStyle::TopRightInfo());
    const std::size_t line_count = std::min<std::size_t>(3, data_.items.size());
    for (std::size_t i = 0; i < line_count; ++i) {
        const auto& item = data_.items[i];
        const std::string line = item.name + "   " + std::to_string(item.price) + "   库存 " + std::to_string(item.stock);
        m_font_renderer->DrawText(window, line, {x, y + row_h * (2.1f + static_cast<float>(i))}, TextStyle::Default());
    }
    m_font_renderer->DrawText(window, data_.player_gold_prefix + " " + std::to_string(data_.player_gold), {x, y + row_h * 5.2f}, TextStyle::CoinText());

    sf::RectangleShape detail_panel({inner_rect.size.x - 24.0f, 24.0f});
    detail_panel.setPosition({x, y + row_h * 6.5f});
    detail_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    detail_panel.setOutlineThickness(1.0f);
    detail_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(detail_panel);
    const std::string desc = data_.selected_item_desc.empty()
        ? data_.selected_item_empty_text
        : data_.selected_item_desc;
    m_font_renderer->DrawText(window, desc, {x + 6.0f, y + row_h * 6.75f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
