#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct ContractPanelViewData {
    int tracking_volume_id = 1;
    int completed_volumes = 0;
    int total_volumes = 6;
    std::string tracking_volume_name = "第一卷";
    std::string tracking_bonus = "完成本卷可解锁奖励";
    std::vector<std::string> task_lines;
    std::string volumes_line_prefix = "卷册: [1][2][3][4][5][6]   总进度:";
    std::string tracking_line_prefix = "当前卷:";
    std::string tracking_name_separator = "卷-";
    std::string bonus_prefix = "章节奖励:";
    std::string tasks_title = "任务条目";
    std::string chapter_reward_hint_text = "章节奖励区: 完成本卷全部任务后解锁";
};

class PixelContractPanel : public PixelUiPanel {
public:
    PixelContractPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const ContractPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    ContractPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
