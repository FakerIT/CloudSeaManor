#include "CloudSeamanor/PixelWorkshopPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

PixelWorkshopPanel::PixelWorkshopPanel()
    : PixelUiPanel({{320.0f, 120.0f}, {640.0f, 480.0f}}, "工坊", true) {
}

void PixelWorkshopPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    m_font_renderer->DrawText(window,
                              data_.tabs_text + "  自动制作: " + std::string(data_.auto_craft ? "ON" : "OFF"),
                              {x, y},
                              TextStyle::PanelTitle());
    m_font_renderer->DrawText(window,
                              "工坊等级: Lv" + std::to_string(data_.workshop_level) +
                                  "  已解锁槽位: " + std::to_string(data_.unlocked_slots),
                              {x, y + row_h},
                              TextStyle::TopRightInfo());
    m_font_renderer->DrawText(window, data_.queue_title_text, {x, y + row_h * 2.2f}, TextStyle::PanelTitle());
    const int progress_pct = static_cast<int>(std::max(0.0f, std::min(1.0f, data_.queue_progress)) * 100.0f);
    if (data_.queue_lines.empty()) {
        m_font_renderer->DrawText(window,
                                  data_.queue_primary_prefix + data_.active_recipe + "      " + std::to_string(progress_pct) + data_.queue_progress_suffix +
                                      std::to_string(std::max(0, data_.queued_output)),
                                  {x, y + row_h * 3.2f},
                                  TextStyle::Default());
        m_font_renderer->DrawText(window, data_.empty_slot_line_2, {x, y + row_h * 4.2f}, TextStyle::Default());
        m_font_renderer->DrawText(window, data_.empty_slot_line_3, {x, y + row_h * 5.2f}, TextStyle::Default());
    } else {
        const std::size_t line_count = std::min<std::size_t>(3, data_.queue_lines.size());
        for (std::size_t i = 0; i < line_count; ++i) {
            m_font_renderer->DrawText(
                window,
                data_.queue_lines[i],
                {x, y + row_h * (3.2f + static_cast<float>(i))},
                TextStyle::Default());
        }
    }

    sf::RectangleShape material_panel({inner_rect.size.x - 24.0f, 26.0f});
    material_panel.setPosition({x, y + row_h * 6.5f});
    material_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    material_panel.setOutlineThickness(1.0f);
    material_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(material_panel);
    m_font_renderer->DrawText(window,
                              data_.stock_prefix + " " + data_.stock_tea_label + " " + std::to_string(data_.tea_leaf_stock)
                                  + "  " + data_.stock_wood_label + " " + std::to_string(data_.wood_stock)
                                  + "  " + data_.stock_crystal_label + " " + std::to_string(data_.crystal_stock),
                              {x + 6.0f, y + row_h * 6.75f},
                              TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.2f}, TextStyle::Default());
}

}  // namespace CloudSeamanor::engine
