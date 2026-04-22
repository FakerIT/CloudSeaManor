#pragma once

#include "CloudSeamanor/PixelProgressBar.hpp"
#include "CloudSeamanor/PixelUiPanel.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct CloudForecastViewData {
    std::string today_state_text;
    std::string tomorrow_state_text;
    int tide_countdown_days = 0;
    int crop_bonus_percent = 0;
    int spirit_bonus = 0;
    std::vector<std::string> recommendations;
};

class PixelCloudSeaForecastPanel : public PixelUiPanel {
public:
    PixelCloudSeaForecastPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void UpdateData(const CloudForecastViewData& data) { m_data = data; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    CloudForecastViewData m_data{};
    PixelProgressBar m_tide_progress;
};

}  // namespace CloudSeamanor::engine
