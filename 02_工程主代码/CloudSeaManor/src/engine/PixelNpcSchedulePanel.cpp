#include "CloudSeamanor/engine/PixelNpcSchedulePanel.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

namespace CloudSeamanor::engine {

namespace {
constexpr float kRowHeight = 28.0f;
constexpr float kIconSize = 16.0f;
constexpr float kPadding = 8.0f;
}

PixelNpcSchedulePanel::PixelNpcSchedulePanel() {
    SetSize({280.0f, 320.0f});
    SetTitle("NPC日程");
}

void PixelNpcSchedulePanel::UpdateData(const NpcSchedulePanelViewData& data) {
    data_ = data;
}

void PixelNpcSchedulePanel::RenderContent(
    sf::RenderWindow& window,
    const sf::FloatRect& inner_rect) {

    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) {
        return;
    }

    float y = inner_rect.position.y + kPadding;

    // 权限提示
    if (!data_.has_permission) {
        TextStyle hint_style = TextStyle::Default();
        hint_style.character_size = 11;
        hint_style.fill_color = ColorPalette::TextBrown;
        hint_style.line_spacing = 1.2f;

        m_font_renderer->DrawText(
            window,
            data_.unlock_hint,
            {inner_rect.position.x + kPadding, y},
            hint_style);
        y += kRowHeight * 1.5f;
    }

    // 可见NPC列表
    TextStyle header_style = TextStyle::Default();
    header_style.character_size = 12;
    header_style.fill_color = ColorPalette::BrownDark;

    m_font_renderer->DrawText(
        window,
        data_.visible_npcs.empty() ? data_.no_npc_visible : "可见NPC:",
        {inner_rect.position.x + kPadding, y},
        header_style);
    y += kRowHeight;

    TextStyle npc_style = TextStyle::Default();
    npc_style.character_size = 11;
    npc_style.fill_color = ColorPalette::TextBrown;
    npc_style.line_spacing = 1.2f;

    for (const auto& npc : data_.visible_npcs) {
        std::string line = "· " + npc.npc_name + " - " + npc.location_name;
        if (!npc.activity.empty() && npc.activity != "idle") {
            line += " [" + npc.activity + "]";
        }

        m_font_renderer->DrawText(
            window,
            line,
            {inner_rect.position.x + kPadding + 8.0f, y},
            npc_style);
        y += kRowHeight * 0.9f;

        if (y > inner_rect.position.y + inner_rect.size.y - kRowHeight) {
            break;
        }
    }

    // 隐藏NPC数量提示
    if (!data_.hidden_npcs.empty()) {
        y += kRowHeight * 0.5f;
        TextStyle hidden_style = TextStyle::Default();
        hidden_style.character_size = 10;
        hidden_style.fill_color = ColorPalette::TextMuted;

        std::string hidden_text = "(还有 " + std::to_string(data_.hidden_npcs.size()) + " 位NPC位置未解锁)";
        m_font_renderer->DrawText(
            window,
            hidden_text,
            {inner_rect.position.x + kPadding + 8.0f, y},
            hidden_style);
    }
}

}  // namespace CloudSeamanor::engine
