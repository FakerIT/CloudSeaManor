#pragma once

#include "CloudSeamanor/PixelArtStyle.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <deque>
#include <string>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

class PixelNotificationBanner {
public:
    void Push(const std::string& message);
    void Update(float delta_seconds);
    void Render(sf::RenderWindow& window, PixelFontRenderer& font_renderer) const;

private:
    struct BannerItem {
        std::string text;
        float timer = 0.0f;
    };

    std::deque<BannerItem> m_queue;
};

}  // namespace CloudSeamanor::engine
