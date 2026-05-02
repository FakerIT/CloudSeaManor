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
// - Render festival decorations (data-driven)
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/rendering/PlayerVisualComponent.hpp"
#include "CloudSeamanor/engine/rendering/FestivalDecorationSystem.hpp"
#include "CloudSeamanor/engine/rendering/ShapePool.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace CloudSeamanor::engine {

class WorldRenderer {
public:
    WorldRenderer();

    void Render(
        sf::RenderWindow& window,
        const GameWorldState& world_state
    );

    // ========================================================================
    // 【LoadFestivalDecorations】加载节日装饰配置
    // ========================================================================
    bool LoadFestivalDecorations(const std::string& config_path);

private:
    void RenderGroundTiles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderObstacles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderInteractables_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderTeaPlots_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderNpcs_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderSpiritBeast_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderParticles_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPickups_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPlacedObjects_(sf::RenderWindow& window, const GameWorldState& ws);
    void RenderPlayer_(sf::RenderWindow& window, const GameWorldState& ws);
    void EnsureTileTextureLoaded_();
    void DrawAtlasEntity_(sf::RenderWindow& window,
                          const sf::IntRect& atlas_rect,
                          const sf::FloatRect& world_rect,
                          const sf::Color& tint = sf::Color::White);

    PlayerVisualComponent player_visual_;
    sf::Texture tiny_town_tilemap_;
    bool tiny_town_tilemap_loaded_ = false;

    // 节日装饰系统（数据驱动）
    rendering::FestivalDecorationSystem festival_decorations_;

    // 形状对象池（用于粒子渲染优化）
    rendering::CircleShapePool circle_pool_{64};
    rendering::RectShapePool rect_pool_{64};
};

}  // namespace CloudSeamanor::engine
