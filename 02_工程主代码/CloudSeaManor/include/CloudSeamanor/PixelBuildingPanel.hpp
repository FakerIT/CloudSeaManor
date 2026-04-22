#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct BuildingPanelViewData {
    int player_gold = 0;
    int main_house_level = 1;
    bool greenhouse_unlocked = false;
    int workshop_level = 1;
    std::string category_prefix = "分类: [全部] [生产] [住宿] [装饰] [功能]  资金:";
    std::string list_title = "建筑列表";
    std::vector<std::string> building_lines;
    std::string upgrade_requirement_text = "升级条件: 金币 8000(满足)  木材 30(满足)  晶石 5(不足)";
    std::string preview_text = "预览: 升级后加工速度 +15%";
    std::string actions_text = "[升级] [查看外观] [取消]";
};

class PixelBuildingPanel : public PixelUiPanel {
public:
    PixelBuildingPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const BuildingPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    BuildingPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
