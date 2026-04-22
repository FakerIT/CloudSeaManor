#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <array>
#include <string>

namespace CloudSeamanor::engine {

class PixelDailyRecommendationPanel : public PixelUiPanel {
public:
    PixelDailyRecommendationPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void SetRecommendations(const std::array<std::string, 3>& items) { m_items = items; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    std::array<std::string, 3> m_items{
        "生产：优先收获高品质作物",
        "社交：寻找关键 NPC 对话",
        "探索：清理周边采集点"
    };
};

}  // namespace CloudSeamanor::engine
