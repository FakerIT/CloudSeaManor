#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct SpiritRealmPanelViewData {
    std::string mode_text = "轻松";
    int remaining_count = 3;
    int max_count = 5;
    int drop_bonus_percent = 0;
    bool in_spirit_realm = false;
    std::string mode_line_prefix = "模式:";
    std::string mode_options_text = "[关闭战斗] [挑战]";
    std::string remaining_line_prefix = "今日剩余次数:";
    std::string drop_bonus_prefix = "当前云海加成: 掉落率 +";
    std::string regions_title = "区域卡片";
    std::vector<std::string> region_lines;
    std::string default_region_line_1 = "浅层云径  Lv8   掉落: 灵尘/雾草";
    std::string default_region_line_2 = "潮汐裂谷  Lv15  状态: 🔒 需契约卷3";
    std::string default_region_line_3 = "霜岚祭坛  Lv22  状态: 🔒 需山庄等级5";
    std::string active_state_in_realm_text = "探索中";
    std::string active_state_unlocked_text = "已解锁";
    std::string active_state_suffix = "  状态: ";
    std::string lock_hint_text = "锁定区域会显示解锁条件与推荐战力";
    std::string actions_text = "[进入浅层] [查看掉落表] [挑战首领]";
};

class PixelSpiritRealmPanel : public PixelUiPanel {
public:
    PixelSpiritRealmPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const SpiritRealmPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    SpiritRealmPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
