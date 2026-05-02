#pragma once

// ============================================================================
// 【PickupSystemRuntime】拾取系统运行时协调器
// ============================================================================
// Responsibilities:
// - Per-frame pickup update (visual animation)
// - Collect nearby pickups into inventory
// - Forward hints to UI layer
// ============================================================================

#include "CloudSeamanor/engine/PickupSystem.hpp"

namespace CloudSeamanor::engine {

class GameWorldState;

class PickupSystemRuntime {
public:
    using HintCallback = std::function<void(const std::string&, float)>;

    PickupSystemRuntime(
        PickupSystem& pickup_system,
        GameWorldState& world_state,
        HintCallback hint_callback
    );

    void Initialize();
    void Update(float delta_seconds);
    void CollectNearby();

private:
    PickupSystem& pickup_system_;
    GameWorldState& world_state_;
    HintCallback hint_callback_;
};

}  // namespace CloudSeamanor::engine
