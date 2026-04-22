#include "CloudSeamanor/PixelTeaGardenPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelTeaGardenPanel::PixelTeaGardenPanel()
    : PixelUiPanel({{360.0f, 140.0f}, {560.0f, 420.0f}}, "茶园管理", true) {
}

void PixelTeaGardenPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "云海状态: 晴 | 品质加成预览", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "[浇水] [施肥] [采摘] [修整]", {x, y + 30.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
