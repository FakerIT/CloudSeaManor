#pragma once

// ============================================================================
// 【SpiritBeastSystem】灵兽行为系统
// ============================================================================
// Responsibilities:
// - Manage spirit beast AI state machine (Follow/Wander/Idle/Interact)
// - Update spirit beast position relative to player
// - Handle patrol point navigation
// - Process idle timers
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"

#include <vector>

namespace CloudSeamanor::engine {

class SpiritBeastSystem {
public:
    void Update(
        GameWorldState& world_state,
        float delta_seconds
    );

    void SetState(SpiritBeastState new_state);

    [[nodiscard]] SpiritBeastState CurrentState() const { return current_state_; }

private:
    void UpdateFollow_(GameWorldState& world_state, float delta_seconds);
    void UpdateWander_(GameWorldState& world_state, float delta_seconds);
    void UpdateIdle_(GameWorldState& world_state, float delta_seconds);

    // P18-PERF-002: 障碍物缓存（避免每帧重复遍历）
    void UpdateObstacleCache_(const GameWorldState& world_state);
    bool IsInCachedObstacle_(const sf::Vector2f& point) const;

    SpiritBeastState current_state_ = SpiritBeastState::Follow;
    float idle_timer_ = 0.0f;
    sf::Vector2f last_player_position_{0.0f, 0.0f};
    bool has_last_player_position_ = false;

    // 障碍物缓存
    std::vector<sf::FloatRect> cached_obstacles_;
    std::size_t cached_obstacle_count_ = 0;
};

}  // namespace CloudSeamanor::engine
