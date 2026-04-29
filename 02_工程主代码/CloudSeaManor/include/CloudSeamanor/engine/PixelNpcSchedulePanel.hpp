#pragma once

// ============================================================================
// 【PixelNpcSchedulePanel】NPC日程查看面板
// ============================================================================
// Responsibilities:
// - P1-002: 在地图界面上显示NPC当前位置
// - 支持按好感度等级解锁查看权限
// - 显示NPC当前位置和活动状态
// ============================================================================

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct NpcLocationEntry {
    std::string npc_id;
    std::string npc_name;
    std::string location_name;
    std::string activity;
    int heart_level = 0;
    bool is_visible = false;
};

struct NpcSchedulePanelViewData {
    std::string title_text = "NPC日程";
    std::string unlock_hint = "好感达到4级后可查看所有NPC位置";
    std::string no_npc_visible = "暂无NPC可见";
    int viewer_heart_level = 0;
    bool has_permission = false;
    std::vector<NpcLocationEntry> visible_npcs;
    std::vector<NpcLocationEntry> hidden_npcs;
};

class PixelNpcSchedulePanel : public PixelUiPanel {
public:
    PixelNpcSchedulePanel();

    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const NpcSchedulePanelViewData& data);

    static constexpr int kRequiredHeartLevelForAllView = 4;

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    NpcSchedulePanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
