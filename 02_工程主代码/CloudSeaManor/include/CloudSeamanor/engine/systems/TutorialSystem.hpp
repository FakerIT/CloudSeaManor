#pragma once

// ============================================================================
// 【TutorialSystem】教程引导系统
// ============================================================================
// Responsibilities:
// - Manage tutorial progress flags (TutorialState)
// - Time-based hint triggers (session time thresholds)
// - Interaction-based hint triggers (highlighted objects)
// - Debug overlay toggle
// ============================================================================

#include "CloudSeamanor/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class TutorialSystem {
public:
    using HintCallback = std::function<void(const std::string&, float)>;

    TutorialSystem(GameWorldState& world_state, HintCallback hint_callback);

    void Reset();
    void Update(float delta_seconds);
    void ToggleDebugOverlay();
    [[nodiscard]] bool IsDebugOverlayVisible() const;

private:
    GameWorldState& world_state_;
    HintCallback hint_callback_;
};

}  // namespace CloudSeamanor::engine
