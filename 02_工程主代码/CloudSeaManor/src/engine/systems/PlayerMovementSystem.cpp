#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【PlayerMovementSystem::Update】每帧更新玩家移动
// ============================================================================
void PlayerMovementSystem::Update(
    GameWorldState& world_state,
    float delta_seconds,
    const sf::Vector2f& direction
) {
    auto& player = world_state.GetPlayer();
    const float speed = world_state.GetConfig().player_speed;
    const sf::Vector2f movement(direction.x * speed * delta_seconds, direction.y * speed * delta_seconds);

    std::vector<CloudSeamanor::domain::RectF> obstacles;
    obstacles.reserve(world_state.GetObstacleBounds().size());
    for (const auto& obstacle : world_state.GetObstacleBounds()) {
        obstacles.push_back(CloudSeamanor::adapter::ToDomain(obstacle));
    }

    player.Move(
        CloudSeamanor::adapter::ToDomain(movement),
        CloudSeamanor::adapter::ToDomain(world_state.GetConfig().world_bounds),
        obstacles);

    is_moving_ = (direction.x != 0.0f || direction.y != 0.0f);
}

}  // namespace CloudSeamanor::engine
