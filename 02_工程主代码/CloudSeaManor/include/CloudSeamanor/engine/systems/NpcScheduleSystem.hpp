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

#include <unordered_map>

namespace CloudSeamanor::engine {

class NpcScheduleSystem {
public:
    bool LoadSchedule(const std::string& csv_path);

    void Update(
        GameWorldState& world_state,
        float delta_seconds
    );

private:
    [[nodiscard]] const std::vector<NpcScheduleEntry>& ResolveScheduleForNpc_(const NpcActor& npc) const;

    void UpdateNpcPosition_(
        NpcActor& npc,
        int current_day,
        int current_minute,
        float delta_seconds
    );

    std::unordered_map<std::string, std::vector<NpcScheduleEntry>> loaded_schedules_;
};

}  // namespace CloudSeamanor::engine
