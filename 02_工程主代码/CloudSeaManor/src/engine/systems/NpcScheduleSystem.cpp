#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/GameAppNpc.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// 【NpcScheduleSystem::LoadSchedule】加载日程表
// ============================================================================
bool NpcScheduleSystem::LoadSchedule(const std::string& csv_path) {
    loaded_schedules_.clear();
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "NpcScheduleSystem: 无法打开日程表，使用现有内存日程。path=" + csv_path);
        return false;
    }

    std::string line;
    int loaded_rows = 0;
    int skipped_rows = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("npc_id,", 0) == 0) continue;

        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            fields.push_back(part);
        }
        if (fields.size() < 6) {
            ++skipped_rows;
            continue;
        }
        const std::string& npc_id = fields[0];
        const std::string& day_type = fields[1];
        if (!(day_type.empty() || day_type == "normal")) {
            ++skipped_rows;
            continue;
        }

        try {
            NpcScheduleEntry entry;
            entry.start_minutes = std::stoi(fields[2]);
            entry.end_minutes = std::stoi(fields[3]);
            if (entry.end_minutes <= entry.start_minutes) {
                ++skipped_rows;
                continue;
            }
            entry.location = fields[4];
            entry.activity = fields[5];
            loaded_schedules_[npc_id].push_back(std::move(entry));
            ++loaded_rows;
        } catch (...) {
            ++skipped_rows;
        }
    }

    for (auto& [_, entries] : loaded_schedules_) {
        std::sort(entries.begin(), entries.end(), [](const NpcScheduleEntry& a, const NpcScheduleEntry& b) {
            return a.start_minutes < b.start_minutes;
        });
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "NpcScheduleSystem: 日程加载完成 loaded_rows=" + std::to_string(loaded_rows)
        + ", skipped_rows=" + std::to_string(skipped_rows)
        + ", npc_count=" + std::to_string(loaded_schedules_.size()));
    return loaded_rows > 0;
}

const std::vector<NpcScheduleEntry>& NpcScheduleSystem::ResolveScheduleForNpc_(const NpcActor& npc) const {
    const auto it = loaded_schedules_.find(npc.id);
    if (it != loaded_schedules_.end() && !it->second.empty()) {
        return it->second;
    }
    return npc.schedule;
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

    const auto& schedule = ResolveScheduleForNpc_(npc);
    for (const auto& entry : schedule) {
        if (current_minute >= entry.start_minutes && current_minute < entry.end_minutes) {
            npc.current_location = entry.location;
            npc.current_activity = entry.activity;
            break;
        }
    }

    const sf::Vector2f target = AnchorForLocation(npc.current_location);
    sf::Vector2f position = CloudSeamanor::adapter::ToSf(npc.position);
    const sf::Vector2f delta = target - position;
    const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (distance > 4.0f) {
        position = position + (delta / distance) * (delta_seconds * 55.0f);
        npc.position = CloudSeamanor::domain::Vec2f{position.x, position.y};
    }

    const float favor_ratio =
        std::clamp(static_cast<float>(npc.favor + 20) / 60.0f, 0.0f, 1.0f);
    const std::uint8_t alpha = static_cast<std::uint8_t>(120 + favor_ratio * 110.0f);
    npc.fill_rgba = PackRgba(
        static_cast<std::uint8_t>(140 + favor_ratio * 90.0f),
        static_cast<std::uint8_t>(130 + favor_ratio * 50.0f),
        static_cast<std::uint8_t>(120 + favor_ratio * 80.0f),
        alpha);
}

// ============================================================================
// 【NpcScheduleSystem::Update】每帧更新 NPC
// ============================================================================
void NpcScheduleSystem::Update(
    GameWorldState& world_state,
    float delta_seconds
) {
    for (auto& npc : world_state.MutableNpcs()) {
        npc.state = NpcState::Patrol;
        const int current_minute = world_state.GetClock().Hour() * 60
                                 + world_state.GetClock().Minute();
        UpdateNpcPosition_(npc, world_state.GetClock().Day(),
                           current_minute, delta_seconds);
    }
}

}  // namespace CloudSeamanor::engine
