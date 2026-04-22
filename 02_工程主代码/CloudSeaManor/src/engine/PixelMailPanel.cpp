#include "CloudSeamanor/PixelMailPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelMailPanel::PixelMailPanel()
    : PixelUiPanel({{400.0f, 160.0f}, {480.0f, 400.0f}}, "邮件与订单", true) {
}

void PixelMailPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "邮件列表: 发件人 / 主题 / 时间", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "[收取物品] [删除邮件]", {x, y + 30.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
