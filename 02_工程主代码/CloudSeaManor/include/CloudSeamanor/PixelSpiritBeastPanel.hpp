#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>

namespace CloudSeamanor::engine {

struct SpiritBeastPanelViewData {
    std::string beast_name = "灵团";
    std::string state_text = "休息";
    int favor = 0;
    bool dispatched = false;
    std::string trait = "Watering Aid";
    std::string category_text = "分类: [全部] [采集型] [守护型] [辅助型] [传说型]  数量: 1";
    std::string cards_title = "灵兽卡片";
    std::string influence_hint_text = "今日互动状态会影响次日产出与协助效率";
    std::string recruit_hint_text = "招募条件: 完成契约第二卷并建造灵兽小屋 Lv2";
    std::string actions_text = "[喂食🍃] [互动💕] [派遣🗺] [休息😴]";
    std::string dispatch_in_progress_text = "进行中";
    std::string dispatch_idle_text = "待命";
};

class PixelSpiritBeastPanel : public PixelUiPanel {
public:
    PixelSpiritBeastPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const SpiritBeastPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    SpiritBeastPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
