#include "CloudSeamanor/PixelBeastiaryPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelBeastiaryPanel::PixelBeastiaryPanel()
    : PixelUiPanel({{340.0f, 120.0f}, {600.0f, 480.0f}}, "灵兽图鉴", true) {
}

void PixelBeastiaryPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "筛选: 全部/采集型/守护型/辅助型/传说型", {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "已发现: 彩色头像  未发现: ???", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
