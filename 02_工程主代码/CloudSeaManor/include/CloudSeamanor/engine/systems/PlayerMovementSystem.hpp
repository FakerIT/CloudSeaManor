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

#include "CloudSeamanor/GameWorldState.hpp"

#include <SFML/System/Vector2.hpp>

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
    bool is_moving_ = false;
};

}  // namespace CloudSeamanor::engine
