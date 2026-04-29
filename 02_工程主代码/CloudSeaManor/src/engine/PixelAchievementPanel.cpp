#include "CloudSeamanor/engine/PixelAchievementPanel.hpp"

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelAchievementPanel::PixelAchievementPanel()
    : PixelUiPanel({{380.0f, 140.0f}, {520.0f, 440.0f}}, "成就", true) {
}

void PixelAchievementPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window,
                              data_.progress_prefix + " " + std::to_string(data_.unlocked_count) + "/" +
                                  std::to_string(std::max(1, data_.total_count)),
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, data_.legend_text, {x, y + row_h}, TextStyle::TopRightInfo());
    const std::size_t unlocked_show = std::min<std::size_t>(2, data_.unlocked_titles.size());
    for (std::size_t i = 0; i < unlocked_show; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.unlocked_mark + data_.unlocked_titles[i],
                                  {x, y + row_h * (2.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }
    const std::size_t locked_show = std::min<std::size_t>(2, data_.locked_titles.size());
    for (std::size_t i = 0; i < locked_show; ++i) {
        m_font_renderer->DrawText(window,
                                  data_.locked_mark + data_.locked_titles[i],
                                  {x, y + row_h * (4.2f + static_cast<float>(i))},
                                  TextStyle::Default());
    }

    sf::RectangleShape unlock_banner({inner_rect.size.x - 24.0f, 24.0f});
    unlock_banner.setPosition({x, y + row_h * 6.5f});
    unlock_banner.setFillColor(ColorPalette::CoinGold);
    unlock_banner.setOutlineThickness(1.0f);
    unlock_banner.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(unlock_banner);
    m_font_renderer->DrawText(window, data_.unlock_banner_text, {x + 6.0f, y + row_h * 6.75f}, TextStyle::HotkeyHint());
}

}  // namespace CloudSeamanor::engine
