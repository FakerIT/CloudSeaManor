#include "CloudSeamanor/engine/PixelShopPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

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
    TextStyle title = TextStyle::PanelTitle();
    TextStyle body = TextStyle::Default();
    TextStyle info = TextStyle::TopRightInfo();
    TextStyle affordable = TextStyle::Default();
    affordable.fill_color = ColorPalette::SuccessGreen;
    TextStyle expensive = TextStyle::Default();
    expensive.fill_color = ColorPalette::DeepBrown;
    TextStyle low_stock = TextStyle::Default();
    low_stock.fill_color = ColorPalette::WarningPink;

    m_font_renderer->DrawText(window, data_.shop_name, {x, y}, title);
    m_font_renderer->DrawText(window, data_.items_header_text, {x, y + row_h}, info);

    sf::RectangleShape list_panel({inner_rect.size.x - 24.0f, 124.0f});
    list_panel.setPosition({x, y + row_h * 1.8f});
    list_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 5));
    list_panel.setOutlineThickness(1.0f);
    list_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(list_panel);

    const std::size_t line_count = std::min<std::size_t>(5, data_.items.size());
    for (std::size_t i = 0; i < line_count; ++i) {
        const auto& item = data_.items[i];
        TextStyle line_style = body;
        if (item.stock <= 1) {
            line_style = low_stock;
        } else if (data_.player_gold >= item.price) {
            line_style = affordable;
        } else {
            line_style = expensive;
        }
        const std::string line =
            std::to_string(i + 1) + ") "
            + item.name + "   " + std::to_string(item.price)
            + "   库存 " + std::to_string(item.stock);
        m_font_renderer->DrawText(window, line, {x + 8.0f, y + row_h * (2.05f + static_cast<float>(i) * 0.9f)}, line_style);
    }

    int affordable_count = 0;
    for (const auto& item : data_.items) {
        if (item.price <= data_.player_gold) ++affordable_count;
    }
    m_font_renderer->DrawText(
        window,
        data_.player_gold_prefix + " " + std::to_string(data_.player_gold)
            + "  可购买商品: " + std::to_string(affordable_count),
        {x, y + row_h * 6.0f},
        TextStyle::CoinText());

    sf::RectangleShape detail_panel({inner_rect.size.x - 24.0f, 24.0f});
    detail_panel.setPosition({x, y + row_h * 7.0f});
    detail_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    detail_panel.setOutlineThickness(1.0f);
    detail_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(detail_panel);
    const std::string desc = data_.selected_item_desc.empty()
        ? data_.selected_item_empty_text
        : data_.selected_item_desc;
    m_font_renderer->DrawText(window, desc, {x + 6.0f, y + row_h * 7.24f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.55f}, body);
}

}  // namespace CloudSeamanor::engine
