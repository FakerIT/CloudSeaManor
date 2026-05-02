#pragma once

// ============================================================================
// 【PlayerMovementSystem】玩家移动系统
// ============================================================================
// Responsibilities:
// - Read WASD/Arrow key input
// - Normalize movement vector
// - Apply player movement with collision detection
// - Handle stamina consumption/recovery during movement
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"

#include <SFML/System/Vector2.hpp>
#include <vector>

namespace CloudSeamanor::engine {

class PlayerMovementSystem {
public:
    void Update(
        GameWorldState& world_state,
        float delta_seconds,
        const sf::Vector2f& direction
    );

    [[nodiscard]] bool IsMoving() const { return is_moving_; }

private:
    // 缓存转换后的障碍物（避免每帧重复转换 sf::FloatRect → domain::RectF）
    std::vector<CloudSeamanor::domain::RectF> cached_obstacles_;
    std::size_t last_obstacle_count_ = 0;

    bool is_moving_ = false;
};

}  // namespace CloudSeamanor::engine
