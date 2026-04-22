#include "CloudSeamanor/PixelContractPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelContractPanel::PixelContractPanel()
    : PixelUiPanel({{280.0f, 80.0f}, {720.0f, 560.0f}}, "云海契约", true) {
}

void PixelContractPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window,
                              data_.volumes_line_prefix + " " + std::to_string(data_.completed_volumes) +
                                  "/" + std::to_string(std::max(1, data_.total_volumes)),
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window,
                              data_.tracking_line_prefix + " 第" + std::to_string(data_.tracking_volume_id)
                                  + data_.tracking_name_separator + data_.tracking_volume_name
                                  + "  " + data_.bonus_prefix + " " + data_.tracking_bonus,
                              {x, y + row_h},
                              TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, data_.tasks_title, {x, y + row_h * 2.2f}, TextStyle::PanelTitle());
    const std::size_t task_count = std::min<std::size_t>(3, data_.task_lines.size());
    for (std::size_t i = 0; i < task_count; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.task_lines[i],
                                  {x, y + row_h * (3.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }

    sf::RectangleShape chapter_reward({inner_rect.size.x - 24.0f, 28.0f});
    chapter_reward.setPosition({x, y + row_h * 6.5f});
    chapter_reward.setFillColor(ColorPalette::HighlightYellow);
    chapter_reward.setOutlineThickness(1.0f);
    chapter_reward.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(chapter_reward);
    m_font_renderer->DrawText(window, data_.chapter_reward_hint_text, {x + 6.0f, y + row_h * 6.8f}, TextStyle::HotkeyHint());
}

}  // namespace CloudSeamanor::engine
