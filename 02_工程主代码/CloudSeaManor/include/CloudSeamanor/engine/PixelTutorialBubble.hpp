#pragma once

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <string>

namespace CloudSeamanor::engine {

class PixelTutorialBubble : public PixelUiPanel {
public:
    PixelTutorialBubble();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void SetStep(int step_index, const std::string& text);

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    int m_step_index = 1;
    std::string m_step_text = "按 WASD 移动角色。";
};

}  // namespace CloudSeamanor::engine
