#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct TeaGardenPlotLineViewData {
    std::string name;
    int progress_percent = 0;
    std::string quality_text;
};

struct TeaGardenPanelViewData {
    std::string cloud_state_text = "未知";
    int spirit_bonus = 0;
    int quality_bonus_percent = 0;
    std::string cloud_preview_text = "晴+0%  雾+0%  浓云+0%  大潮+0%";
    std::vector<TeaGardenPlotLineViewData> plots;
    std::string cloud_state_prefix = "云海状态:";
    std::string spirit_bonus_prefix = "灵气:+";
    std::string quality_bonus_prefix = "品质率:+";
    std::string cloud_preview_prefix = "云海加成预览:";
    std::string plots_title = "茶田状态";
    std::string quality_hint_text = "品质: 普通(灰) 优质(绿) 珍品(蓝) 圣品(金)";
    std::string actions_text = "[💧浇水] [🌿施肥] [🍃采摘] [🔧修整]";
};

class PixelTeaGardenPanel : public PixelUiPanel {
public:
    PixelTeaGardenPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const TeaGardenPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    TeaGardenPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
