#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct WorkshopPanelViewData {
    bool auto_craft = false;
    int workshop_level = 1;
    int unlocked_slots = 1;
    std::string active_recipe = "暂无";
    float queue_progress = 0.0f;
    int queued_output = 0;
    int tea_leaf_stock = 0;
    int wood_stock = 0;
    int crystal_stock = 0;
    std::string tabs_text = "[制茶] [加工] [酿造] [精炼]";
    std::string queue_title_text = "当前制作队列";
    std::vector<std::string> queue_lines;
    std::string queue_primary_prefix = "1) ";
    std::string queue_progress_suffix = "%  排队:";
    std::string empty_slot_line_2 = "2) 空槽位";
    std::string empty_slot_line_3 = "3) 空槽位";
    std::string stock_prefix = "库存:";
    std::string stock_tea_label = "灵茶叶";
    std::string stock_wood_label = "木柴";
    std::string stock_crystal_label = "晶粉";
    std::string actions_text = "[开始制作] [取消制作]";
};

class PixelWorkshopPanel : public PixelUiPanel {
public:
    PixelWorkshopPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const WorkshopPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    WorkshopPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
