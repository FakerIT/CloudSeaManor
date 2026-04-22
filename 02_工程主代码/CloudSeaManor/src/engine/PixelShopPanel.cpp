#include "CloudSeamanor/PixelShopPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelShopPanel::PixelShopPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "商店", true) {
}

void PixelShopPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "商品列表: 名称 / 价格 / 库存", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "玩家金币: 0", {x, y + 28.0f}, TextStyle::CoinText());
    m_font_renderer->DrawText(window, "[购买] [取消]", {x, y + 56.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
