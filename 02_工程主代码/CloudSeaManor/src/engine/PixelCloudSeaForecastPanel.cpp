#include "CloudSeamanor/PixelCloudSeaForecastPanel.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

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

    m_font_renderer->DrawText(window, "今日云海: " + m_data.today_state_text, {x, y}, TextStyle::Default());
    m_font_renderer->DrawText(window, "明日预报: " + m_data.tomorrow_state_text, {x, y + line_h}, TextStyle::Default());
    m_font_renderer->DrawText(window,
                              "作物加成 +" + std::to_string(m_data.crop_bonus_percent) + "% | 灵气 +" + std::to_string(m_data.spirit_bonus),
                              {x, y + line_h * 2.0f},
                              TextStyle::TopRightInfo());

    const int clamped_days = std::max(0, std::min(7, m_data.tide_countdown_days));
    const float ratio = 1.0f - static_cast<float>(clamped_days) / 7.0f;
    m_tide_progress.SetPosition({x, y + line_h * 3.4f});
    m_tide_progress.SetProgress(ratio);
    m_tide_progress.Render(window);
    m_font_renderer->DrawText(window, "大潮倒计时: " + std::to_string(m_data.tide_countdown_days) + " 天", {x, y + line_h * 4.0f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, "今日推荐:", {x, y + line_h * 5.2f}, TextStyle::PanelTitle());
    const std::size_t count = std::min<std::size_t>(m_data.recommendations.size(), 3);
    for (std::size_t i = 0; i < count; ++i) {
        m_font_renderer->DrawText(window,
                                  std::to_string(i + 1) + ". " + m_data.recommendations[i],
                                  {x, y + line_h * (6.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }
}

}  // namespace CloudSeamanor::engine
