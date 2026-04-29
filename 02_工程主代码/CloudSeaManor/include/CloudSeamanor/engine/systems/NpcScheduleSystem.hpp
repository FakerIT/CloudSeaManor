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

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

class NpcScheduleSystem {
public:
    bool LoadSchedule(const std::string& csv_path);

    void Update(
        GameWorldState& world_state,
        float delta_seconds
    );

    /**
     * @brief 获取所有可见 NPC 的当前位置信息（用于日程面板显示）
     * @param npcs NPC 列表引用
     * @param min_heart_level_to_show 最小好感等级才显示位置
     * @return 可见 NPC 位置信息列表
     */
    [[nodiscard]] std::vector<NpcLocationEntry> GetVisibleNpcLocations(
        const std::vector<NpcActor>& npcs,
        int min_heart_level_to_show
    ) const;

private:
    static constexpr float kOffscreenDistanceThreshold = 420.0f;
    static constexpr float kOffscreenUpdateIntervalSeconds = 0.45f;

    struct NpcScheduleOverrideEntry {
        int stage = 0;
        std::string season;
        std::string weather;
        std::string time_block;
        std::string anchor_id;
        std::string behavior_tag;
    };

    [[nodiscard]] const std::vector<NpcScheduleEntry>& ResolveScheduleForNpc_(const NpcActor& npc) const;
    [[nodiscard]] bool ApplyStageOverride_(
        NpcActor& npc,
        int current_minute,
        CloudSeamanor::domain::Season season,
        CloudSeamanor::domain::CloudState cloud_state) const;

    void UpdateNpcPosition_(
        NpcActor& npc,
        int current_day,
        int current_minute,
        CloudSeamanor::domain::Season season,
        CloudSeamanor::domain::CloudState cloud_state,
        bool festival_active,
        float delta_seconds
    );

    std::unordered_map<std::string, std::vector<NpcScheduleEntry>> loaded_schedules_;
    std::unordered_map<std::string, std::vector<NpcScheduleOverrideEntry>> schedule_overrides_;
    std::unordered_map<std::string, float> offscreen_update_accumulator_;
};

}  // namespace CloudSeamanor::engine
