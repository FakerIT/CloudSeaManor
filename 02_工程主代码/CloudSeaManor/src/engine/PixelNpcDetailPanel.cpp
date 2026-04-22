#include "CloudSeamanor/PixelNpcDetailPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelNpcDetailPanel::PixelNpcDetailPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "NPC 社交详情", true) {
}

void PixelNpcDetailPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "好感度: ♥♥♥♡♡♡♡♡♡♡", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "[对话] [送礼] [查看喜好] [约会]", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
