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
    std::string today_prefix = "今日云海:";
    std::string tomorrow_prefix = "明日预报:";
    std::string bonus_format_prefix = "作物加成 +";
    std::string bonus_midfix = "% | 灵气 +";
    std::string tide_countdown_prefix = "大潮倒计时:";
    std::string tide_countdown_suffix = "天";
    std::string recommendations_title = "今日推荐:";
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
