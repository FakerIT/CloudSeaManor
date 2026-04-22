#pragma once

// ============================================================================
// 【NpcScheduleSystem】NPC日程系统
// ============================================================================
// Responsibilities:
// - Load and parse NPC schedule data from CSV
// - Update NPC positions based on current time
// - Interpolate NPC movement between schedule points
// - Apply affection-based visual effects (transparency)
// ============================================================================

#include "CloudSeamanor/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class NpcScheduleSystem {
public:
    bool LoadSchedule(const std::string& csv_path);

    void Update(
        GameWorldState& world_state,
        float delta_seconds
    );

private:
    void UpdateNpcPosition_(
        NpcActor& npc,
        int current_day,
        int current_minute,
        float delta_seconds
    );
};

}  // namespace CloudSeamanor::engine
