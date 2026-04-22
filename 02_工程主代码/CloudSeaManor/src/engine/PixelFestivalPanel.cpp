#include "CloudSeamanor/PixelFestivalPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelFestivalPanel::PixelFestivalPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "节日活动", true) {
}

void PixelFestivalPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "节日倒计时: 3 天", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "[前往活动地点] [查看奖励] [轻松参与]", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
