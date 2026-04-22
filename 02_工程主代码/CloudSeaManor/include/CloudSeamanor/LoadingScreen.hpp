#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <string>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

class LoadingScreen {
public:
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void SetStageText(const std::string& text) { m_stage_text = text; }
    void Update(float delta_seconds);
    void Render(sf::RenderWindow& window, PixelFontRenderer* font_renderer) const;

private:
    bool m_visible = false;
    float m_timer = 0.0f;
    std::string m_stage_text = "正在加载...";
};

}  // namespace CloudSeamanor::engine
