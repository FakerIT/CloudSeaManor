#include "CloudSeamanor/engine/PixelContextMenu.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>

namespace CloudSeamanor::engine {

PixelContextMenu::PixelContextMenu()
    : PixelUiPanel({{0.0f, 0.0f}, {180.0f, 132.0f}}, "", false) {
    SetVisible(false);
}

void PixelContextMenu::OpenAt(const sf::Vector2f& position, const std::vector<MenuItem>& items) {
    m_items = items;
    if (m_items.size() > 4) {
        m_items.resize(4);
    }
    const float item_h = 28.0f;
    SetRect({position, {180.0f, std::max(36.0f, item_h * static_cast<float>(m_items.size()))}});
    Open();
    SetVisible(true);
    m_hovered_index = -1;
}

void PixelContextMenu::CloseMenu() {
    m_hovered_index = -1;
    Close();
    SetVisible(false);
}

void PixelContextMenu::HandleMouseMove(float mx, float my) {
    if (!IsVisible()) return;
    const auto r = GetRect();
    if (!r.contains({mx, my})) {
        m_hovered_index = -1;
        return;
    }
    const float rel = my - r.position.y;
    m_hovered_index = std::clamp(static_cast<int>(rel / 28.0f), 0, static_cast<int>(m_items.size()) - 1);
}

bool PixelContextMenu::HandleMouseClick(float mx, float my) {
    if (!IsVisible()) return false;
    const auto r = GetRect();
    if (!r.contains({mx, my})) {
        CloseMenu();
        return false;
    }
    if (m_hovered_index >= 0 && m_hovered_index < static_cast<int>(m_items.size())) {
        if (m_on_select) {
            m_on_select(m_items[static_cast<std::size_t>(m_hovered_index)].id);
        }
        CloseMenu();
        return true;
    }
    return false;
}

void PixelContextMenu::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (m_font_renderer == nullptr || !m_font_renderer->IsLoaded()) return;
    const float item_h = 28.0f;
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        const float row_y = inner_rect.position.y + static_cast<float>(i) * item_h;
        if (static_cast<int>(i) == m_hovered_index) {
            sf::RectangleShape row;
            row.setPosition({inner_rect.position.x, row_y});
            row.setSize({inner_rect.size.x, item_h - 2.0f});
            row.setFillColor(ColorPalette::HighlightYellow);
            window.draw(row, sf::RenderStates::Default);
        }
        m_font_renderer->DrawText(window, m_items[i].label, {inner_rect.position.x + 8.0f, row_y + 6.0f}, TextStyle::Default());
    }
}

}  // namespace CloudSeamanor::engine
