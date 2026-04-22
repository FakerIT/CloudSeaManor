#pragma once

// ============================================================================
// 【WorldRenderer】世界渲染器
// ============================================================================
// Responsibilities:
// - Render ground tiles
// - Render obstacle shapes
// - Render interactable objects
// - Render tea plots
// - Render NPCs
// - Render spirit beast
// - Render heart particles
// - Render pickups
// - Render player
// ============================================================================

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/engine/rendering/PlayerVisualComponent.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

namespace CloudSeamanor::engine {

class WorldRenderer {
public:
    WorldRenderer();

    void Render(
        sf::RenderWindow& window,
        const GameWorldState& world_state
    );

private:
    void RenderGroundTiles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderObstacles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderInteractables_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderTeaPlots_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderNpcs_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderSpiritBeast_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderParticles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPickups_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPlayer_(sf::RenderWindow& window, const GameWorldState& ws);

    PlayerVisualComponent player_visual_;
};

}  // namespace CloudSeamanor::engine
