#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct FestivalPanelViewData {
    std::string active_name = "暂无节日";
    int countdown_days = 0;
    int completed_events = 0;
    int total_events = 3;
    std::vector<std::string> upcoming;
    std::string reward_text = "奖励: 祭典徽章 / 金币 1200 / 限定装饰";
    std::string upcoming_prefix = "预告: ";
    std::string upcoming_empty_text = "暂无";
    std::string participation_text = "参与模式: 轻松参与(80%) / 全力参与(100%)";
    std::string selected_participation_text = "当前选择: 轻松参与 ✓";
    std::string actions_text = "[前往活动地点] [查看奖励详情] [轻松参与✓]";
};

class PixelFestivalPanel : public PixelUiPanel {
public:
    PixelFestivalPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const FestivalPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    FestivalPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
