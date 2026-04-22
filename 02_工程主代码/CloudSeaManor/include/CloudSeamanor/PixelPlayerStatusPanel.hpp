#pragma once

#include "CloudSeamanor/PixelProgressBar.hpp"
#include "CloudSeamanor/PixelUiPanel.hpp"

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
