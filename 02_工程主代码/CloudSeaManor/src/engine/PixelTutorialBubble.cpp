#include "CloudSeamanor/engine/PixelTutorialBubble.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelTutorialBubble::PixelTutorialBubble()
    : PixelUiPanel({{300.0f, 560.0f}, {320.0f, 120.0f}}, "新手引导", false) {
}

void PixelTutorialBubble::SetStep(int step_index, const std::string& text) {
    m_step_index = step_index;
    m_step_text = text;
}

void PixelTutorialBubble::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 8.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "步骤 " + std::to_string(m_step_index) + "/11", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawWrappedText(window, m_step_text, {x, y + 24.0f}, inner_rect.size.x - 16.0f, TextStyle::Default(), 1.4f);
    m_font_renderer->DrawText(window, "[上一条]  [下一条 ->]", {x, y + 74.0f}, TextStyle::HotkeyHint());
}

}  // namespace CloudSeamanor::engine
