#include "CloudSeamanor/PixelDailyRecommendationPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

namespace CloudSeamanor::engine {

PixelDailyRecommendationPanel::PixelDailyRecommendationPanel()
    : PixelUiPanel({{20.0f, 220.0f}, {260.0f, 140.0f}}, "今日推荐三件事", false) {
}

void PixelDailyRecommendationPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 8.0f;
    const float y = inner_rect.position.y + 8.0f;
    m_font_renderer->DrawText(window, "1. " + m_items[0], {x, y}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "2. " + m_items[1], {x, y + 24.0f}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, "3. " + m_items[2], {x, y + 48.0f}, TextStyle::TopRightInfo());
}

}  // namespace CloudSeamanor::engine
