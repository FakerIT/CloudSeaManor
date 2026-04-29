#pragma once

#include "CloudSeamanor/engine/PixelArtStyle.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <deque>
#include <string>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

class PixelNotificationBanner {
public:
    void SetTimings(float fade_in_seconds, float hold_seconds, float fade_out_seconds);
    void SetCloudReportDuration(float total_seconds);
    void Push(const std::string& message);
    void Update(float delta_seconds);
    void Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const;
    [[nodiscard]] bool IsCloudReportBannerHit(float mx, float my, float window_width) const;

private:
    static constexpr float kWidth = 600.0f;
    static constexpr float kHeight = 36.0f;
    static constexpr float kTop = 16.0f;
    static constexpr float kSpacing = 4.0f;

    struct BannerItem {
        std::string text;
        float timer = 0.0f;
        float total_duration = 3.6f;
    };

    std::deque<BannerItem> m_queue;
    float fade_in_seconds_ = 0.3f;
    float hold_seconds_ = 3.0f;
    float fade_out_seconds_ = 0.3f;
    float cloud_report_total_seconds_ = 4.0f;
};

}  // namespace CloudSeamanor::engine
