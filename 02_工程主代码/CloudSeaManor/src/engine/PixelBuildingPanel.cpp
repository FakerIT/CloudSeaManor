#include "CloudSeamanor/PixelBuildingPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelBuildingPanel::PixelBuildingPanel()
    : PixelUiPanel({{320.0f, 100.0f}, {640.0f, 520.0f}}, "建筑升级", true) {
}

void PixelBuildingPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window,
                              data_.category_prefix + " " + std::to_string(data_.player_gold),
                              {x, y},
                              TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, data_.list_title, {x, y + row_h}, TextStyle::PanelTitle());
    if (data_.building_lines.empty()) {
        m_font_renderer->DrawText(window,
                                  "主屋 Lv" + std::to_string(data_.main_house_level) + "   状态: 可升级   效果: 解锁更多系统",
                                  {x, y + row_h * 2.1f},
                                  TextStyle::Default());
        m_font_renderer->DrawText(window,
                                  "温室 " + std::string(data_.greenhouse_unlocked ? "已解锁" : "未解锁") + "   状态: " +
                                      (data_.greenhouse_unlocked ? "可使用" : "需主屋升级"),
                                  {x, y + row_h * 3.1f},
                                  TextStyle::Default());
        m_font_renderer->DrawText(window,
                                  "工坊 Lv" + std::to_string(data_.workshop_level) + "   状态: 可升级   效果: 队列 +1",
                                  {x, y + row_h * 4.1f},
                                  TextStyle::Default());
    } else {
        const std::size_t count = std::min<std::size_t>(3, data_.building_lines.size());
        for (std::size_t i = 0; i < count; ++i) {
            m_font_renderer->DrawText(
                window,
                data_.building_lines[i],
                {x, y + row_h * (2.1f + static_cast<float>(i))},
                TextStyle::Default());
        }
    }
    m_font_renderer->DrawText(window, data_.upgrade_requirement_text, {x, y + row_h * 5.2f}, TextStyle::Default());

    sf::RectangleShape effect_panel({inner_rect.size.x - 24.0f, 24.0f});
    effect_panel.setPosition({x, y + row_h * 6.5f});
    effect_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    effect_panel.setOutlineThickness(1.0f);
    effect_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(effect_panel);
    m_font_renderer->DrawText(window, data_.preview_text, {x + 6.0f, y + row_h * 6.75f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.1f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
