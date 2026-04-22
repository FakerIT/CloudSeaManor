#include "CloudSeamanor/PixelBuildingPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelBuildingPanel::PixelBuildingPanel()
    : PixelUiPanel({{320.0f, 100.0f}, {640.0f, 520.0f}}, "建筑升级", true) {
}

void PixelBuildingPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "分类: 全部 / 生产 / 住宿 / 装饰 / 功能", {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "材料满足: 绿色  不满足: 红色", {x, y + 26.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
