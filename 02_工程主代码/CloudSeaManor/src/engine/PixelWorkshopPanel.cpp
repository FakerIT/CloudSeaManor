#include "CloudSeamanor/PixelWorkshopPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelWorkshopPanel::PixelWorkshopPanel()
    : PixelUiPanel({{320.0f, 120.0f}, {640.0f, 480.0f}}, "工坊", true) {
}

void PixelWorkshopPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "[制茶] [加工] [酿造] [精炼]", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "队列: 1/3  材料不足项高亮", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
