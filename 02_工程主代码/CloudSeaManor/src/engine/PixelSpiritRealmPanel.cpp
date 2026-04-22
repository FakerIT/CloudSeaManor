#include "CloudSeamanor/PixelSpiritRealmPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelSpiritRealmPanel::PixelSpiritRealmPanel()
    : PixelUiPanel({{340.0f, 120.0f}, {600.0f, 480.0f}}, "灵界探索", true) {
}

void PixelSpiritRealmPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "模式: [轻松] [关闭战斗] [挑战]", {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "今日剩余次数: 3/5", {x, y + 26.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
