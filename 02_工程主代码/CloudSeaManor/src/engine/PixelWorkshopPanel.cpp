#include "CloudSeamanor/PixelWorkshopPanel.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>
#include <array>

namespace CloudSeamanor::engine {

PixelWorkshopPanel::PixelWorkshopPanel()
    : PixelUiPanel({{320.0f, 120.0f}, {640.0f, 480.0f}}, "工坊", true) {
}

void PixelWorkshopPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float x = inner_rect.position.x + 12.0f;
    const float y = inner_rect.position.y + 8.0f;
    const float row_h = 22.0f;
    const int progress_pct = static_cast<int>(std::clamp(data_.queue_progress, 0.0f, 1.0f) * 100.0f);
    int processing_count = 0;
    for (const auto& line : data_.queue_lines) {
        if (line.find("加工中") != std::string::npos || line.find("processing") != std::string::npos) {
            ++processing_count;
        }
    }
    const int utilization = (data_.unlocked_slots > 0)
        ? static_cast<int>((100.0f * static_cast<float>(processing_count)) / static_cast<float>(data_.unlocked_slots))
        : 0;

    TextStyle title = TextStyle::PanelTitle();
    TextStyle info = TextStyle::TopRightInfo();
    TextStyle body = TextStyle::Default();
    TextStyle warn = TextStyle::Default();
    warn.fill_color = ColorPalette::DeepBrown;
    TextStyle hot = TextStyle::HotkeyHint();

    m_font_renderer->DrawText(window, data_.tabs_text, {x, y}, title);
    m_font_renderer->DrawText(
        window,
        "自动制作: " + std::string(data_.auto_craft ? "ON" : "OFF")
            + "  Lv" + std::to_string(data_.workshop_level)
            + "  槽位 " + std::to_string(data_.unlocked_slots)
            + "  利用率 " + std::to_string(std::clamp(utilization, 0, 999)) + "%",
        {x, y + row_h},
        info);

    // Queue block
    sf::RectangleShape queue_panel({inner_rect.size.x - 24.0f, 104.0f});
    queue_panel.setPosition({x, y + row_h * 2.1f});
    queue_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 5));
    queue_panel.setOutlineThickness(1.0f);
    queue_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(queue_panel);
    m_font_renderer->DrawText(window, data_.queue_title_text, {x + 8.0f, y + row_h * 2.25f}, title);

    if (data_.queue_lines.empty()) {
        m_font_renderer->DrawText(
            window,
            data_.queue_primary_prefix + data_.active_recipe + "  "
                + std::to_string(progress_pct) + data_.queue_progress_suffix + std::to_string(std::max(0, data_.queued_output)),
            {x + 8.0f, y + row_h * 3.15f},
            body);
        m_font_renderer->DrawText(window, data_.empty_slot_line_2, {x + 8.0f, y + row_h * 4.05f}, body);
        m_font_renderer->DrawText(window, data_.empty_slot_line_3, {x + 8.0f, y + row_h * 4.95f}, body);
    } else {
        const std::size_t line_count = std::min<std::size_t>(3, data_.queue_lines.size());
        for (std::size_t i = 0; i < line_count; ++i) {
            TextStyle line_style = body;
            if (data_.queue_lines[i].find("空槽位") != std::string::npos) {
                line_style.fill_color = ColorPalette::DarkGray;
            } else if (data_.queue_lines[i].find("加工中") != std::string::npos) {
                line_style.fill_color = ColorPalette::SuccessGreen;
            }
            m_font_renderer->DrawText(
                window,
                data_.queue_lines[i],
                {x + 8.0f, y + row_h * (3.15f + static_cast<float>(i) * 0.9f)},
                line_style);
        }
    }

    // Material and status block
    sf::RectangleShape material_panel({inner_rect.size.x - 24.0f, 48.0f});
    material_panel.setPosition({x, y + row_h * 7.05f});
    material_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    material_panel.setOutlineThickness(1.0f);
    material_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(material_panel);

    m_font_renderer->DrawText(
        window,
        data_.stock_prefix + " " + data_.stock_tea_label + " " + std::to_string(data_.tea_leaf_stock)
            + "  " + data_.stock_wood_label + " " + std::to_string(data_.wood_stock)
            + "  " + data_.stock_crystal_label + " " + std::to_string(data_.crystal_stock),
        {x + 6.0f, y + row_h * 7.25f},
        hot);

    const bool any_low_stock = (data_.tea_leaf_stock <= 0 || data_.wood_stock <= 0 || data_.crystal_stock <= 0);
    m_font_renderer->DrawText(
        window,
        any_low_stock ? "状态: 材料不足，部分队列将暂停" : "状态: 材料充足，可持续生产",
        {x + 6.0f, y + row_h * 8.05f},
        any_low_stock ? warn : body);

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 9.2f}, body);
}

}  // namespace CloudSeamanor::engine
