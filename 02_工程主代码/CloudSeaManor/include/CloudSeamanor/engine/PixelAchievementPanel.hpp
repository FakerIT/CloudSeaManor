#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct AchievementPanelViewData {
    int unlocked_count = 0;
    int total_count = 0;
    std::vector<std::string> unlocked_titles;
    std::vector<std::string> locked_titles;
    std::string progress_prefix = "已解锁:";
    std::string legend_text = "已解锁(金边) / 未解锁(灰色半透明)";
    std::string unlocked_mark = "★ ";
    std::string locked_mark = "☆ ";
    std::string unlock_banner_text = "解锁成就时将触发通知横幅";
};

class PixelAchievementPanel : public PixelUiPanel {
public:
    PixelAchievementPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const AchievementPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    AchievementPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
