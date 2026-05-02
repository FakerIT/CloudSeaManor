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
    auto& player = world_state.MutablePlayer();
    const float speed = world_state.GetConfig().player_speed;
    const sf::Vector2f movement(direction.x * speed * delta_seconds, direction.y * speed * delta_seconds);

    // 障碍物缓存：仅在障碍物数量变化时重建缓存
    const auto& source_obstacles = world_state.GetObstacleBounds();
    if (cached_obstacles_.size() != source_obstacles.size()) {
        cached_obstacles_.clear();
        cached_obstacles_.reserve(source_obstacles.size());
        for (const auto& obstacle : source_obstacles) {
            cached_obstacles_.push_back(CloudSeamanor::adapter::ToDomain(obstacle));
        }
        last_obstacle_count_ = source_obstacles.size();
    }

    player.Move(
        CloudSeamanor::adapter::ToDomain(movement),
        CloudSeamanor::adapter::ToDomain(world_state.GetConfig().world_bounds),
        cached_obstacles_);

    is_moving_ = (direction.x != 0.0f || direction.y != 0.0f);
}

}  // namespace CloudSeamanor::engine
