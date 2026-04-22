#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>

namespace CloudSeamanor::engine {

struct NpcDetailPanelViewData {
    std::string name = "未知NPC";
    int heart_level = 0;
    int favor = 0;
    bool talked_today = false;
    bool gifted_today = false;
    std::string location = "未知地点";
    std::string title_suffix = "社交详情";
    std::string location_prefix = "当前地点:";
    std::string favor_prefix = "好感度:";
    std::string heart_event_text = "心事件: ✓H1  ✓H2  ●H3  ○H4  ○H5  ○H6  ○H7  ○H8  ○H9  ○H10";
    std::string talked_done_text = "已对话";
    std::string talked_todo_text = "未对话";
    std::string gifted_done_text = "今日已送";
    std::string gifted_todo_text = "可送礼";
    std::string event_hint_text = "可触发事件: 茶田东侧观景桥 (16:00 后)";
    std::string actions_text = "[💬对话] [🎁送礼] [📖查看喜好] [💒约会]";
};

class PixelNpcDetailPanel : public PixelUiPanel {
public:
    PixelNpcDetailPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const NpcDetailPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    NpcDetailPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
