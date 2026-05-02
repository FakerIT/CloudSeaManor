#include "CloudSeamanor/engine/PixelToolUpgradePanel.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PixelToolUpgradePanel】构造函数
// ============================================================================
PixelToolUpgradePanel::PixelToolUpgradePanel()
    : PixelUiPanel({{340.0f, 100.0f}, {480.0f, 520.0f}}, "工具工坊", true) {
}

// ============================================================================
// 【SetFontRenderer】
// ============================================================================
void PixelToolUpgradePanel::SetFontRenderer(const PixelFontRenderer* renderer) {
    m_font_renderer = renderer;
}

// ============================================================================
// 【UpdateData】
// ============================================================================
void PixelToolUpgradePanel::UpdateData(const ToolUpgradeViewData& data) {
    m_data = data;
}

// ============================================================================
// 【SetOnUpgradeCallback】
// ============================================================================
void PixelToolUpgradePanel::SetOnUpgradeCallback(
    std::function<bool(int tool_index)> callback
) {
    on_upgrade_callback_ = std::move(callback);
}

// ============================================================================
// 【SetOnTabChanged】
// ============================================================================
void PixelToolUpgradePanel::SetOnTabChanged(
    std::function<void(ToolPanelTab)> callback
) {
    on_tab_changed_callback_ = std::move(callback);
}

// ============================================================================
// 【HandleKeyPress】
// ============================================================================
void PixelToolUpgradePanel::HandleKeyPress(int key) {
    if (active_tab_ == ToolPanelTab::Tools) {
        // 工具面板：上下选择，Enter升级
        if (key == 0) {  // 上
            selected_tool_index_ = (selected_tool_index_ + 6) % 7;
        } else if (key == 1) {  // 下
            selected_tool_index_ = (selected_tool_index_ + 1) % 7;
        } else if (key == 2) {  // Enter
            if (on_upgrade_callback_) {
                on_upgrade_callback_(selected_tool_index_);
            }
        }
    } else if (active_tab_ == ToolPanelTab::Sprinklers || active_tab_ == ToolPanelTab::Fertilizer) {
        // 上下滚动
        if (key == 0) {
            // 上滚动
        } else if (key == 1) {
            // 下滚动
        }
    }
}

// ============================================================================
// 【SetActiveTab】
// ============================================================================
void PixelToolUpgradePanel::SetActiveTab(ToolPanelTab tab) {
    active_tab_ = tab;
    selected_tool_index_ = 0;
}

// ============================================================================
// 【RenderContent】
// ============================================================================
void PixelToolUpgradePanel::RenderContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;

    // 渲染标签页
    RenderTabs(window, inner_rect);

    // 渲染内容区
    switch (active_tab_) {
        case ToolPanelTab::Tools:
            RenderToolsContent(window, inner_rect);
            break;
        case ToolPanelTab::Sprinklers:
            RenderSprinklersContent(window, inner_rect);
            break;
        case ToolPanelTab::Fertilizer:
            RenderFertilizerContent(window, inner_rect);
            break;
        default:
            break;
    }

    // 渲染底部信息
    float y = inner_rect.position.y + inner_rect.size.y - 40.0f;
    m_font_renderer->DrawText(
        window,
        m_data.player_gold_text,
        {inner_rect.position.x + kLeftMargin, y},
        TextStyle::CoinText()
    );
}

// ============================================================================
// 【RenderTabs】
// ============================================================================
void PixelToolUpgradePanel::RenderTabs(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    const float tab_width = (inner_rect.size.x - 24.0f) / 3.0f;
    const char* tab_names[] = {"工具升级", "洒水器", "肥料"};

    for (std::uint8_t i = 0; i < 3; ++i) {
        float tab_x = inner_rect.position.x + 12.0f + i * tab_width;
        float tab_y = inner_rect.position.y + 4.0f;

        bool is_active = (active_tab_ == static_cast<ToolPanelTab>(i));

        // 标签背景
        sf::RectangleShape tab_bg({tab_width - 2.0f, kTabHeight});
        tab_bg.setPosition({tab_x, tab_y});
        if (is_active) {
            tab_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 10));
        } else {
            tab_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
        }
        tab_bg.setOutlineThickness(1.0f);
        tab_bg.setOutlineColor(ColorPalette::BrownOutline);
        window.draw(tab_bg);

        // 标签文字
        TextStyle tab_style = TextStyle::Default();
        if (is_active) {
            tab_style.fill_color = ColorPalette::DeepBrown;
        }
        m_font_renderer->DrawText(
            window,
            tab_names[i],
            {tab_x + 8.0f, tab_y + 4.0f},
            tab_style
        );
    }
}

// ============================================================================
// 【RenderToolsContent】
// ============================================================================
void PixelToolUpgradePanel::RenderToolsContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    float y = inner_rect.position.y + kTopMargin;
    const float x = inner_rect.position.x + kLeftMargin;

    // 列标题
    m_font_renderer->DrawText(window, "工具", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "等级", {x + 100.0f, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "经验", {x + 160.0f, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(window, "效果", {x + 230.0f, y}, TextStyle::PanelTitle());

    y += 24.0f;

    for (int i = 0; i < 7; ++i) {
        const auto& tool = m_data.tools[i];
        bool is_selected = (i == selected_tool_index_);

        // 行背景
        sf::RectangleShape row_bg({inner_rect.size.x - 24.0f, kRowHeight - 2.0f});
        row_bg.setPosition({x - 4.0f, y});
        if (is_selected) {
            row_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 8));
        } else {
            row_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
        }
        row_bg.setOutlineThickness(is_selected ? 2.0f : 1.0f);
        row_bg.setOutlineColor(is_selected ? ColorPalette::SuccessGreen : ColorPalette::BrownOutline);
        window.draw(row_bg);

        // 工具名称
        m_font_renderer->DrawText(window, tool.name, {x, y + 8.0f}, TextStyle::Default());

        // 等级
        TextStyle tier_style = TextStyle::Default();
        tier_style.fill_color = tool.is_max_level ? ColorPalette::FireOrange : ColorPalette::DeepBrown;
        m_font_renderer->DrawText(window, tool.tier_name, {x + 100.0f, y + 8.0f}, tier_style);

        // 经验条
        sf::RectangleShape exp_bg({60.0f, 12.0f});
        exp_bg.setPosition({x + 155.0f, y + 10.0f});
        exp_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
        exp_bg.setOutlineThickness(1.0f);
        exp_bg.setOutlineColor(ColorPalette::BrownOutline);
        window.draw(exp_bg);

        float exp_width = 58.0f * tool.exp_ratio;
        sf::RectangleShape exp_fill({exp_width, 10.0f});
        exp_fill.setPosition({x + 156.0f, y + 11.0f});
        exp_fill.setFillColor(ColorPalette::FreshGreen);
        window.draw(exp_fill);

        // 效果
        TextStyle eff_style = TextStyle::Default();
        eff_style.fill_color = ColorPalette::FreshGreen;
        m_font_renderer->DrawText(window, tool.effect_text, {x + 230.0f, y + 8.0f}, eff_style);

        y += kRowHeight;
    }

    // 提示
    y += 8.0f;
    TextStyle hint_style = TextStyle::HotkeyHint();
    hint_style.fill_color = ColorPalette::StoneGray;
    m_font_renderer->DrawText(window, m_data.hint_text, {x, y}, hint_style);
}

// ============================================================================
// 【RenderSprinklersContent】
// ============================================================================
void PixelToolUpgradePanel::RenderSprinklersContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    float y = inner_rect.position.y + kTopMargin;
    const float x = inner_rect.position.x + kLeftMargin;

    // 标题
    m_font_renderer->DrawText(window, "洒水器", {x, y}, TextStyle::PanelTitle());
    m_font_renderer->DrawText(
        window,
        "已安装: " + std::to_string(m_data.installed_count),
        {x + 200.0f, y},
        TextStyle::Default()
    );

    y += 28.0f;

    for (int i = 0; i < 3; ++i) {
        const auto& sprinkler = m_data.sprinklers[i];

        // 行背景
        sf::RectangleShape row_bg({inner_rect.size.x - 24.0f, kRowHeight + 4.0f});
        row_bg.setPosition({x - 4.0f, y});
        row_bg.setFillColor(ColorPalette::Lighten(ColorPalette::Cream, 2));
        row_bg.setOutlineThickness(1.0f);
        row_bg.setOutlineColor(ColorPalette::BrownOutline);
        window.draw(row_bg);

        // 名称
        m_font_renderer->DrawText(window, sprinkler.name, {x, y + 4.0f}, TextStyle::Default());

        // 范围
        TextStyle range_style = TextStyle::Default();
        range_style.fill_color = ColorPalette::OceanBlue;
        m_font_renderer->DrawText(window, sprinkler.coverage, {x + 100.0f, y + 4.0f}, range_style);

        // 费用
        TextStyle cost_style = TextStyle::HotkeyHint();
        if (sprinkler.can_build) {
            cost_style.fill_color = ColorPalette::SuccessGreen;
        } else {
            cost_style.fill_color = ColorPalette::FireRed;
        }
        m_font_renderer->DrawText(window, sprinkler.cost, {x + 160.0f, y + 4.0f}, cost_style);

        // 构建按钮
        if (sprinkler.can_build) {
            TextStyle build_style = TextStyle::Default();
            build_style.fill_color = ColorPalette::SuccessGreen;
            m_font_renderer->DrawText(window, "[B]建造", {x + 280.0f, y + 10.0f}, build_style);
        }

        y += kRowHeight + 8.0f;
    }

    // 提示
    y += 8.0f;
    TextStyle hint_style = TextStyle::HotkeyHint();
    hint_style.fill_color = ColorPalette::StoneGray;
    m_font_renderer->DrawText(window, "在空地上按B键建造洒水器", {x, y}, hint_style);
}

// ============================================================================
// 【RenderFertilizerContent】
// ============================================================================
void PixelToolUpgradePanel::RenderFertilizerContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect
) {
    float y = inner_rect.position.y + kTopMargin;
    const float x = inner_rect.position.x + kLeftMargin;

    // 标题
    m_font_renderer->DrawText(window, "肥料制作", {x, y}, TextStyle::PanelTitle());

    y += 28.0f;

    // 提示信息
    TextStyle info_style = TextStyle::HotkeyHint();
    info_style.fill_color = ColorPalette::StoneGray;
    m_font_renderer->DrawText(window, "种植前使用肥料可加速生长", {x, y}, info_style);

    y += 30.0f;

    // 提示
    TextStyle hint_style = TextStyle::HotkeyHint();
    hint_style.fill_color = ColorPalette::DeepBrown;
    m_font_renderer->DrawText(window, "肥料可在商店购买或使用材料制作", {x, y}, hint_style);
}

}  // namespace CloudSeamanor::engine
