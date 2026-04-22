#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct BeastiaryPanelViewData {
    int discovered_count = 0;
    int total_count = 4;
    std::vector<std::string> discovered_lines;
    std::vector<std::string> undiscovered_lines;
    std::string filter_text = "筛选: [全部] [采集型] [守护型] [辅助型] [传说型]";
    std::string progress_prefix = "图鉴进度:";
    std::string selected_detail = "选中详情: 暂无";
};

class PixelBeastiaryPanel : public PixelUiPanel {
public:
    PixelBeastiaryPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const BeastiaryPanelViewData& data) { data_ = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
    BeastiaryPanelViewData data_{};
};

}  // namespace CloudSeamanor::engine
