#pragma once

// ============================================================================
// 【RuntimeState】运行时状态（Legacy 兼容层）
// ============================================================================
// Responsibilities:
// - Store UI-only runtime flags/timers that are NOT part of game save state.
// - Kept for backward compatibility with the deprecated GameAppState path.
//
// NOTE:
// - Do NOT store gameplay state here (Player/Inventory/World/Interaction/etc).
// - All gameplay state must live in engine::GameWorldState.
// ============================================================================

#include <string>

namespace CloudSeamanor::game_state {

struct RuntimeState {
    // UI-only timers/flags (not saved)
    float ui_blink_timer = 0.0f;
    float ui_transition_timer = 0.0f;
    bool ui_input_locked = false;

    // Optional: last UI message (debug/telemetry)
    std::string last_ui_message;
};

}  // namespace CloudSeamanor::game_state
