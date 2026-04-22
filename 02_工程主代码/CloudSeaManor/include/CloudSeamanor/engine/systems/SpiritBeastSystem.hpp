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

#include "CloudSeamanor/GameWorldState.hpp"

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

    SpiritBeastState current_state_ = SpiritBeastState::Follow;
    float idle_timer_ = 0.0f;
};

}  // namespace CloudSeamanor::engine
