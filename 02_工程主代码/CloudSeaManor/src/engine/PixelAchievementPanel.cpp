#include "CloudSeamanor/PixelAchievementPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelAchievementPanel::PixelAchievementPanel()
    : PixelUiPanel({{380.0f, 140.0f}, {520.0f, 440.0f}}, "成就", true) {
}

void PixelAchievementPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "已解锁: 0/20", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "已解锁(金边) / 未解锁(灰色)", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
