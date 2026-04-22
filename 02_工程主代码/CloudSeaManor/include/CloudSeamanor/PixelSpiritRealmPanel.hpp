#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

namespace CloudSeamanor::engine {

class PixelSpiritRealmPanel : public PixelUiPanel {
public:
    PixelSpiritRealmPanel();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;
    const PixelFontRenderer* m_font_renderer = nullptr;
};

}  // namespace CloudSeamanor::engine
