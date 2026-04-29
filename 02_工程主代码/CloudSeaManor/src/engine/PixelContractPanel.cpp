#include "CloudSeamanor/engine/PixelContractPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

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
    float cursor_y = y;
    m_font_renderer->DrawText(window,
                              data_.volumes_line_prefix + " " + std::to_string(data_.completed_volumes) +
                                  "/" + std::to_string(std::max(1, data_.total_volumes)),
                              {x, cursor_y},
                              TextStyle::PanelTitle());
    cursor_y += row_h;
    m_font_renderer->DrawText(window,
                              data_.tracking_line_prefix + " 第" + std::to_string(data_.tracking_volume_id)
                                  + data_.tracking_name_separator + data_.tracking_volume_name
                                  + "  " + data_.bonus_prefix + " " + data_.tracking_bonus,
                              {x, cursor_y},
                              TextStyle::TopRightInfo());
    cursor_y += row_h * 1.1f;
    m_font_renderer->DrawText(window, data_.today_recommendation_title, {x, cursor_y}, TextStyle::PanelTitle());
    cursor_y += row_h * 0.9f;
    if (!data_.today_recommendation_text.empty()) {
        m_font_renderer->DrawText(window, data_.today_recommendation_text, {x, cursor_y}, TextStyle::Default());
        cursor_y += row_h * 1.2f;
    } else {
        cursor_y += row_h * 0.3f;
    }
    if (!data_.unlock_hint_text.empty()) {
        TextStyle hint = TextStyle::HotkeyHint();
        hint.fill_color = ColorPalette::WarningPink;
        m_font_renderer->DrawText(window, data_.unlock_hint_text, {x, cursor_y}, hint);
        cursor_y += row_h * 1.2f;
    }

    m_font_renderer->DrawText(window, data_.tasks_title, {x, cursor_y}, TextStyle::PanelTitle());
    cursor_y += row_h;
    const std::size_t task_count = std::min<std::size_t>(3, data_.task_lines.size());
    for (std::size_t i = 0; i < task_count; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.task_lines[i],
                                  {x, cursor_y + row_h * static_cast<float>(i)},
                                  TextStyle::Default());
    }

    sf::RectangleShape chapter_reward({inner_rect.size.x - 24.0f, 28.0f});
    chapter_reward.setPosition({x, y + inner_rect.size.y - 44.0f});
    chapter_reward.setFillColor(ColorPalette::HighlightYellow);
    chapter_reward.setOutlineThickness(1.0f);
    chapter_reward.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(chapter_reward);
    m_font_renderer->DrawText(window, data_.chapter_reward_hint_text, {x + 6.0f, y + inner_rect.size.y - 40.0f}, TextStyle::HotkeyHint());
}

}  // namespace CloudSeamanor::engine
