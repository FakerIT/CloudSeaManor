#include "CloudSeamanor/PixelContractPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelContractPanel::PixelContractPanel()
    : PixelUiPanel({{280.0f, 80.0f}, {720.0f, 560.0f}}, "云海契约", true) {
}

void PixelContractPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "卷册: [1][2][3][4][5][6]", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "任务状态: ✓ 完成  ● 进行中  ○ 未开始", {x, y + 28.0f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
