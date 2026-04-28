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
    TextStyle title = TextStyle::PanelTitle();
    TextStyle body = TextStyle::Default();
    TextStyle info = TextStyle::TopRightInfo();
    TextStyle ok = TextStyle::Default();
    ok.fill_color = ColorPalette::SuccessGreen;
    TextStyle warn = TextStyle::Default();
    warn.fill_color = ColorPalette::DeepBrown;

    m_font_renderer->DrawText(
        window,
        data_.category_prefix + " " + std::to_string(data_.player_gold),
        {x, y},
        info);
    m_font_renderer->DrawText(window, data_.list_title, {x, y + row_h}, title);

    // Building list card
    sf::RectangleShape list_panel({inner_rect.size.x - 24.0f, 108.0f});
    list_panel.setPosition({x, y + row_h * 1.8f});
    list_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 5));
    list_panel.setOutlineThickness(1.0f);
    list_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(list_panel);

    if (data_.building_lines.empty()) {
        m_font_renderer->DrawText(window,
                                  "主屋 Lv" + std::to_string(data_.main_house_level) + "   状态: 可升级   效果: 解锁更多系统",
                                  {x + 8.0f, y + row_h * 2.05f},
                                  body);
        m_font_renderer->DrawText(window,
                                  "温室 " + std::string(data_.greenhouse_unlocked ? "已解锁" : "未解锁") + "   状态: " +
                                      (data_.greenhouse_unlocked ? "可使用" : "需主屋升级"),
                                  {x + 8.0f, y + row_h * 2.95f},
                                  body);
        m_font_renderer->DrawText(window,
                                  "工坊 Lv" + std::to_string(data_.workshop_level) + "   状态: 可升级   效果: 队列 +1",
                                  {x + 8.0f, y + row_h * 3.85f},
                                  body);
    } else {
        const std::size_t count = std::min<std::size_t>(5, data_.building_lines.size());
        for (std::size_t i = 0; i < count; ++i) {
            TextStyle line_style = body;
            if (data_.building_lines[i].find("已解锁") != std::string::npos
                || data_.building_lines[i].find("可使用") != std::string::npos) {
                line_style = ok;
            } else if (data_.building_lines[i].find("未解锁") != std::string::npos
                       || data_.building_lines[i].find("需") != std::string::npos) {
                line_style = warn;
            }
            m_font_renderer->DrawText(
                window,
                data_.building_lines[i],
                {x + 8.0f, y + row_h * (2.05f + static_cast<float>(i) * 0.9f)},
                line_style);
        }
    }

    const int building_progress = std::clamp(
        (data_.main_house_level - 1) * 25
            + (data_.greenhouse_unlocked ? 25 : 0)
            + std::max(0, data_.workshop_level - 1) * 15,
        0, 100);
    m_font_renderer->DrawText(
        window,
        "建设进度: " + std::to_string(building_progress) + "%  主屋Lv" + std::to_string(data_.main_house_level)
            + "  工坊Lv" + std::to_string(data_.workshop_level),
        {x, y + row_h * 5.05f},
        info);

    m_font_renderer->DrawText(window, data_.upgrade_requirement_text, {x, y + row_h * 5.95f}, body);

    sf::RectangleShape effect_panel({inner_rect.size.x - 24.0f, 24.0f});
    effect_panel.setPosition({x, y + row_h * 7.05f});
    effect_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 4));
    effect_panel.setOutlineThickness(1.0f);
    effect_panel.setOutlineColor(ColorPalette::BrownOutline);
    window.draw(effect_panel);
    m_font_renderer->DrawText(window, data_.preview_text, {x + 6.0f, y + row_h * 7.28f}, TextStyle::HotkeyHint());

    m_font_renderer->DrawText(window, data_.actions_text, {x, y + row_h * 8.65f}, body);

    // ===== 温室详细信息（如果已解锁）=====
    if (data_.greenhouse_unlocked) {
        sf::RectangleShape greenhouse_panel({inner_rect.size.x - 24.0f, 68.0f});
        greenhouse_panel.setPosition({x, y + row_h * 9.8f});
        greenhouse_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Season::SpringGreen, 10));
        greenhouse_panel.setOutlineThickness(1.5f);
        greenhouse_panel.setOutlineColor(ColorPalette::Season::SpringGreen);
        window.draw(greenhouse_panel);

        m_font_renderer->DrawText(window, "【温室系统】", {x + 8.0f, y + row_h * 10.0f}, title);
        
        TextStyle green_tip = TextStyle::Default();
        green_tip.fill_color = ColorPalette::Season::SpringGreen;
        m_font_renderer->DrawText(
            window,
            "温室效果：作物生长+10%，不受季节枯萎影响",
            {x + 8.0f, y + row_h * 10.6f},
            green_tip);
        m_font_renderer->DrawText(
            window,
            "使用：在温室门口交互，激活后将新播种地块标记为温室地块",
            {x + 8.0f, y + row_h * 11.2f},
            body);
    } else {
        // 温室解锁提示
        sf::RectangleShape greenhouse_panel({inner_rect.size.x - 24.0f, 68.0f});
        greenhouse_panel.setPosition({x, y + row_h * 9.8f});
        greenhouse_panel.setFillColor(ColorPalette::Lighten(ColorPalette::Season::SummerOrange, 15));
        greenhouse_panel.setOutlineThickness(1.5f);
        greenhouse_panel.setOutlineColor(ColorPalette::Season::SummerOrange);
        window.draw(greenhouse_panel);

        TextStyle lock_tip = TextStyle::Default();
        lock_tip.fill_color = ColorPalette::Season::SummerOrange;
        m_font_renderer->DrawText(window, "【温室未解锁】", {x + 8.0f, y + row_h * 10.0f}, title);
        m_font_renderer->DrawText(
            window,
            "解锁条件：主屋升级至 Lv.3",
            {x + 8.0f, y + row_h * 10.6f},
            lock_tip);
        m_font_renderer->DrawText(
            window,
            "效果：作物不受季节枯萎，生长速度+10%",
            {x + 8.0f, y + row_h * 11.2f},
            body);
    }
}

}  // namespace CloudSeamanor::engine
