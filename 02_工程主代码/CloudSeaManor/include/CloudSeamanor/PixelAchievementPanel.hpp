#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

namespace CloudSeamanor::engine {

class PixelAchievementPanel : public PixelUiPanel {
public:
    PixelAchievementPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
};

}  // namespace CloudSeamanor::engine
