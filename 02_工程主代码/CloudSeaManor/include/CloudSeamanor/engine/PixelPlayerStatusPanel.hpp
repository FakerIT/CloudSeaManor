#pragma once

#include "CloudSeamanor/engine/PixelProgressBar.hpp"
#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>

namespace CloudSeamanor::engine {

struct PlayerStatusViewData {
    std::string player_name;
    int player_level = 1;
    int manor_stage = 1;
    int total_gold = 0;
    float stamina_ratio = 1.0f;
    float spirit_ratio = 1.0f;
    float fatigue_ratio = 0.0f;
    int contract_progress = 0;
    std::string header_level_separator = "Lv.";
    std::string manor_stage_prefix = "山庄阶段";
    std::string total_gold_prefix = "总资产:";
    std::string stamina_label = "体力";
    std::string spirit_label = "灵气";
    std::string fatigue_label = "疲劳";
    std::string contract_progress_prefix = "契约进度:";
    int contract_total = 6;
};

class PixelPlayerStatusPanel : public PixelUiPanel {
public:
    PixelPlayerStatusPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const PlayerStatusViewData& data) { m_data = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    PlayerStatusViewData m_data{};
    PixelProgressBar m_stamina;
    PixelProgressBar m_spirit;
    PixelProgressBar m_fatigue;
};

}  // namespace CloudSeamanor::engine
