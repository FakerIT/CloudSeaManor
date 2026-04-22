#include "CloudSeamanor/PixelSpiritBeastPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelSpiritBeastPanel::PixelSpiritBeastPanel()
    : PixelUiPanel({{320.0f, 100.0f}, {640.0f, 520.0f}}, "灵兽小屋", true) {
}

void PixelSpiritBeastPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "分类: 全部 / 采集型 / 守护型 / 辅助型 / 传说型", {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "[喂食] [互动] [派遣] [休息]", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
