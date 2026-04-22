#pragma once

// ============================================================================
// 【HudRenderer】HUD渲染器
// ============================================================================
// Responsibilities:
// - Render HUD panel (time, day, season, stamina)
// - Render inventory panel
// - Render dialogue panel
// - Render hint bar
// - Render debug text (if enabled)
// ============================================================================

#include "CloudSeamanor/UISystem.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

namespace CloudSeamanor::engine {

class HudRenderer {
public:
    HudRenderer(UISystem& ui_system);

    void Render(
        sf::RenderWindow& window,
        const GameWorldState& world_state
    );

private:
    void RenderBackgroundLayer_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPanels_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderTexts_(sf::RenderWindow& window, const GameWorldState& ws);

    UISystem& ui_system_;
};

}  // namespace CloudSeamanor::engine
