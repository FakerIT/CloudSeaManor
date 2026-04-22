#pragma once

#include "CloudSeamanor/PixelUiPanel.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

class PixelContextMenu : public PixelUiPanel {
public:
    struct MenuItem {
        std::string id;
        std::string label;
    };

    PixelContextMenu();
    void SetFontRenderer(const PixelFontRenderer* renderer) { m_font_renderer = renderer; }
    void OpenAt(const sf::Vector2f& position, const std::vector<MenuItem>& items);
    void CloseMenu();
    void HandleMouseMove(float mx, float my);
    bool HandleMouseClick(float mx, float my);
    void SetOnSelect(std::function<void(const std::string&)> cb) { m_on_select = std::move(cb); }

private:
    void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) override;

    const PixelFontRenderer* m_font_renderer = nullptr;
    std::vector<MenuItem> m_items;
    int m_hovered_index = -1;
    std::function<void(const std::string&)> m_on_select;
};

}  // namespace CloudSeamanor::engine
