#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/GameAppNpc.hpp"

#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// 【NpcScheduleSystem::LoadSchedule】加载日程表（占位）
// ============================================================================
bool NpcScheduleSystem::LoadSchedule(const std::string& /*csv_path*/) {
    return true;
}

// ============================================================================
// 【NpcScheduleSystem::UpdateNpcPosition_】更新单个 NPC 位置
// ============================================================================
void NpcScheduleSystem::UpdateNpcPosition_(
    NpcActor& npc,
    int /*current_day*/,
    int current_minute,
    float delta_seconds
) {
    // 22:00 - 06:00 视为夜间，NPC 回家且隐藏。
    if (current_minute >= 22 * 60 || current_minute < 6 * 60) {
        npc.visible = false;
        return;
    }
    npc.visible = true;

    for (const auto& entry : npc.schedule) {
        if (current_minute >= entry.start_minutes && current_minute < entry.end_minutes) {
            npc.current_location = entry.location;
            npc.current_activity = entry.activity;
            break;
        }
    }

    const sf::Vector2f target = AnchorForLocation(npc.current_location);
    sf::Vector2f position = npc.shape.getPosition();
    const sf::Vector2f delta = target - position;
    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (distance > 4.0f) {
        npc.shape.setPosition(
            position + (delta / distance) * (delta_seconds * 55.0f));
    }

    const float favor_ratio =
        std::clamp(static_cast<float>(npc.favor + 20) / 60.0f, 0.0f, 1.0f);
    const std::uint8_t alpha = static_cast<std::uint8_t>(120 + favor_ratio * 110.0f);
    npc.shape.setFillColor(sf::Color(
        static_cast<std::uint8_t>(140 + favor_ratio * 90.0f),
        static_cast<std::uint8_t>(130 + favor_ratio * 50.0f),
        static_cast<std::uint8_t>(120 + favor_ratio * 80.0f),
        alpha));
}

// ============================================================================
// 【NpcScheduleSystem::Update】每帧更新 NPC
// ============================================================================
void NpcScheduleSystem::Update(
    GameWorldState& world_state,
    float delta_seconds
) {
    for (auto& npc : world_state.GetNpcs()) {
        npc.state = NpcState::Patrol;
        const int current_minute = world_state.GetClock().Hour() * 60
                                 + world_state.GetClock().Minute();
        UpdateNpcPosition_(npc, world_state.GetClock().Day(),
                           current_minute, delta_seconds);
    }
}

}  // namespace CloudSeamanor::engine
