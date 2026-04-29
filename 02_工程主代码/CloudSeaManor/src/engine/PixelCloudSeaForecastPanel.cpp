#include "CloudSeamanor/engine/PixelCloudSeaForecastPanel.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelCloudSeaForecastPanel::PixelCloudSeaForecastPanel()
    : PixelUiPanel({{360.0f, 120.0f}, {560.0f, 480.0f}}, "云海预报", true) {
    m_tide_progress.SetContractStyle();
}

void PixelCloudSeaForecastPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;

    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float line_h = 22.0f;

    m_font_renderer->DrawText(window, m_data.today_prefix + " " + m_data.today_state_text, {x, y}, TextStyle::Default());
    m_font_renderer->DrawText(window, m_data.tomorrow_prefix + " " + m_data.tomorrow_state_text, {x, y + line_h}, TextStyle::Default());
    m_font_renderer->DrawText(window, m_data.aura_stage_prefix + " " + m_data.aura_stage_text, {x, y + line_h * 2.0f}, TextStyle::Default());
    m_font_renderer->DrawText(window, m_data.tide_type_prefix + " " + m_data.tide_type_text, {x, y + line_h * 3.0f}, TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window,
                              m_data.bonus_format_prefix + std::to_string(m_data.crop_bonus_percent)
                                  + m_data.bonus_midfix + std::to_string(m_data.spirit_bonus),
                              {x, y + line_h * 4.0f},
                              TextStyle::TopRightInfo());

    const int clamped_days = std::max(0, std::min(7, m_data.tide_countdown_days));
    const float ratio = 1.0f - static_cast<float>(clamped_days) / 7.0f;
    m_tide_progress.SetPosition({x, y + line_h * 5.4f});
    m_tide_progress.SetProgress(ratio);
    m_tide_progress.Render(window);
    m_font_renderer->DrawText(window,
                              m_data.tide_countdown_prefix + " " + std::to_string(m_data.tide_countdown_days) + " " + m_data.tide_countdown_suffix,
                              {x, y + line_h * 6.0f},
                              TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, m_data.recommendations_title, {x, y + line_h * 7.2f}, TextStyle::PanelTitle());
    const std::size_t count = std::min<std::size_t>(m_data.recommendations.size(), 3);
    for (std::size_t i = 0; i < count; ++i) {
        m_font_renderer->DrawText(window,
                                  std::to_string(i + 1) + ". " + m_data.recommendations[i],
                                  {x, y + line_h * (8.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }
}

}  // namespace CloudSeamanor::engine
