#include "CloudSeamanor/engine/PixelNotificationBanner.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <algorithm>
#include <cstdint>

namespace CloudSeamanor::engine {

void PixelNotificationBanner::SetTimings(float fade_in_seconds, float hold_seconds, float fade_out_seconds) {
    fade_in_seconds_ = std::clamp(fade_in_seconds, 0.05f, 2.0f);
    hold_seconds_ = std::clamp(hold_seconds, 0.1f, 10.0f);
    fade_out_seconds_ = std::clamp(fade_out_seconds, 0.05f, 2.0f);
}

void PixelNotificationBanner::SetCloudReportDuration(float total_seconds) {
    const float min_total = fade_in_seconds_ + 0.2f + fade_out_seconds_;
    cloud_report_total_seconds_ = std::max(min_total, total_seconds);
}

void PixelNotificationBanner::Push(const std::string& message) {
    if (m_queue.size() >= 3) {
        m_queue.pop_front();
    }
    const float default_duration = fade_in_seconds_ + hold_seconds_ + fade_out_seconds_;
    const bool is_cloud_report = message.find("☁ 今日云海") != std::string::npos;
    m_queue.push_back({message, 0.0f, is_cloud_report ? cloud_report_total_seconds_ : default_duration});
}

void PixelNotificationBanner::Update(float delta_seconds) {
    for (auto& item : m_queue) {
        item.timer += delta_seconds;
    }
    while (!m_queue.empty() && m_queue.front().timer > m_queue.front().total_duration) {
        m_queue.pop_front();
    }
}

void PixelNotificationBanner::Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const {
    const float window_width = static_cast<float>(window.getSize().x);
    const float x = (window_width - kWidth) * 0.5f;

    for (std::size_t i = 0; i < m_queue.size(); ++i) {
        const auto& item = m_queue[i];
        const float y = kTop + static_cast<float>(i) * (kHeight + kSpacing);
        const float t = item.timer;
        float alpha = 1.0f;
        if (t < fade_in_seconds_) alpha = t / fade_in_seconds_;
        if (t > item.total_duration - fade_out_seconds_) alpha = (item.total_duration - t) / fade_out_seconds_;
        if (alpha <= 0.0f) continue;

        const sf::Color fill = sf::Color(ColorPalette::Cream.r, ColorPalette::Cream.g, ColorPalette::Cream.b, static_cast<std::uint8_t>(220.0f * alpha));
        const sf::Color top_strip = sf::Color(ColorPalette::LightBrown.r, ColorPalette::LightBrown.g, ColorPalette::LightBrown.b, static_cast<std::uint8_t>(255.0f * alpha));

        sf::VertexArray panel(sf::PrimitiveType::Triangles);
        auto AddQuad = [&](const sf::Vector2f& p0, const sf::Vector2f& p1, const sf::Vector2f& p2, const sf::Vector2f& p3, const sf::Color& color) {
            panel.append(sf::Vertex(p0, color));
            panel.append(sf::Vertex(p1, color));
            panel.append(sf::Vertex(p2, color));
            panel.append(sf::Vertex(p0, color));
            panel.append(sf::Vertex(p2, color));
            panel.append(sf::Vertex(p3, color));
        };
        AddQuad({x, y}, {x + kWidth, y}, {x + kWidth, y + kHeight}, {x, y + kHeight}, fill);
        AddQuad({x, y}, {x + kWidth, y}, {x + kWidth, y + 2.0f}, {x, y + 2.0f}, top_strip);
        window.draw(panel, sf::RenderStates::Default);

        font_renderer.DrawText(window, item.text, {x + 12.0f, y + 10.0f}, TextStyle::Default());
    }
}

bool PixelNotificationBanner::IsCloudReportBannerHit(float mx, float my, float window_width) const {
    const float x = (window_width - kWidth) * 0.5f;
    for (std::size_t i = 0; i < m_queue.size(); ++i) {
        const auto& item = m_queue[i];
        if (item.text.find(std::string("☁ 今日云海")) == std::string::npos) {
            continue;
        }
        const float y = kTop + static_cast<float>(i) * (kHeight + kSpacing);
        const sf::FloatRect rect{{x, y}, {kWidth, kHeight}};
        if (rect.contains({mx, my})) {
            return true;
        }
    }
    return false;
}

}  // namespace CloudSeamanor::engine
