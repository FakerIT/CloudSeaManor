#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/app/GameAppNpc.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/engine/PixelNpcSchedulePanel.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace CloudSeamanor::engine {
namespace {

bool IsKnownLocationId_(const std::string& location) {
    static const std::unordered_set<std::string> kKnownLocationIds = {
        "TeaField", "MainHouse", "Market", "Dock", "Workshop", "VillageCenter", "SpiritGarden", "Observatory",
        "square", "bridge_shelter", "mixed_store", "river_rent_house", "yunseng_shop", "yunseng_tower_house",
        "main_house", "manor_workshop", "observatory", "village", "manor"
    };
    return kKnownLocationIds.contains(location);
}

void SanitizeNpcLocation_(NpcActor& npc) {
    if (npc.current_location.empty() || IsKnownLocationId_(npc.current_location)) {
        return;
    }
    CloudSeamanor::infrastructure::Logger::Warning(
        "NpcScheduleSystem: invalid anchor_id '" + npc.current_location + "' for npc=" + npc.id
        + ", fallback to VillageCenter");
    npc.current_location = "VillageCenter";
    if (npc.current_activity.empty()) {
        npc.current_activity = "relocating";
    }
}

std::string WeatherTagFor_(
    CloudSeamanor::domain::Season season,
    CloudSeamanor::domain::CloudState cloud_state) {
    using CloudSeamanor::domain::CloudState;
    if (cloud_state == CloudState::Clear) {
        return "Clear";
    }
    if (cloud_state == CloudState::Tide) {
        return "Tide";
    }
    if (season == CloudSeamanor::domain::Season::Winter) {
        return "Snow";
    }
    return "Rain";
}

}  // namespace

// ============================================================================
// 【NpcScheduleSystem::LoadSchedule】加载日程表
// ============================================================================
bool NpcScheduleSystem::LoadSchedule(const std::string& csv_path) {
    loaded_schedules_.clear();
    schedule_overrides_.clear();
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
    {
        std::ifstream override_file("assets/data/npc/npc_development_schedule_overrides.csv");
        std::string override_line;
        while (std::getline(override_file, override_line)) {
            if (override_line.empty() || override_line[0] == '#') continue;
            if (override_line.rfind("NpcId,", 0) == 0) continue;
            std::vector<std::string> fields;
            std::stringstream ss(override_line);
            std::string part;
            while (std::getline(ss, part, ',')) {
                fields.push_back(part);
            }
            if (fields.size() < 8 || fields[0].empty()) continue;
            NpcScheduleOverrideEntry entry;
            entry.stage = std::max(0, std::atoi(fields[1].c_str()));
            entry.season = fields[2];
            entry.weather = fields[3];
            entry.time_block = fields[4];
            entry.anchor_id = fields[6];
            entry.behavior_tag = fields[7];
            schedule_overrides_[fields[0]].push_back(std::move(entry));
        }
    }
    return loaded_rows > 0;
}
bool NpcScheduleSystem::ApplyStageOverride_(
    NpcActor& npc,
    int current_minute,
    CloudSeamanor::domain::Season season,
    CloudSeamanor::domain::CloudState cloud_state) const {
    const auto it = schedule_overrides_.find(npc.id);
    if (it == schedule_overrides_.end()) {
        return false;
    }
    const std::string season_name = CloudSeamanor::domain::GameClock::SeasonName(season);
    const std::string weather_tag = WeatherTagFor_(season, cloud_state);
    const std::string time_block =
        current_minute < 12 * 60 ? "Morning" :
        current_minute < 18 * 60 ? "Afternoon" :
        current_minute < 22 * 60 ? "Evening" : "Night";
    for (const auto& row : it->second) {
        if (row.stage != npc.development_stage) continue;
        if (!row.season.empty() && row.season != "Any" && row.season != season_name) continue;
        if (!row.weather.empty() && row.weather != "Any" && row.weather != weather_tag) continue;
        if (!row.time_block.empty() && row.time_block != "Any" && row.time_block != time_block) continue;
        if (!row.anchor_id.empty()) {
            npc.current_location = row.anchor_id;
        }
        if (!row.behavior_tag.empty()) {
            npc.current_activity = row.behavior_tag;
        }
        SanitizeNpcLocation_(npc);
        return true;
    }
    return false;
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
    CloudSeamanor::domain::Season season,
    CloudSeamanor::domain::CloudState cloud_state,
    bool festival_active,
    float delta_seconds
) {
    // 22:00 - 06:00 视为夜间，NPC 回家且隐藏。
    if (current_minute >= 22 * 60 || current_minute < 6 * 60) {
        npc.visible = false;
        return;
    }
    npc.visible = true;

    if (festival_active) {
        // P10-GOV-007: festival day uses a single readable gathering rule.
        npc.current_location = "square";
        npc.current_activity = "festival_gathering";
    } else if (cloud_state == CloudSeamanor::domain::CloudState::Tide) {
        // P1-002: tide day collective gathering for higher world readability.
        npc.current_location = "square";
        npc.current_activity = "tide_gathering";
    } else if (!ApplyStageOverride_(npc, current_minute, season, cloud_state)) {
        const auto& schedule = ResolveScheduleForNpc_(npc);
        for (const auto& entry : schedule) {
            if (current_minute >= entry.start_minutes && current_minute < entry.end_minutes) {
                npc.current_location = entry.location;
                npc.current_activity = entry.activity;
                break;
            }
        }
    }
    SanitizeNpcLocation_(npc);

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
    const int current_minute = world_state.GetClock().Hour() * 60
                             + world_state.GetClock().Minute();
    const auto cloud_state = world_state.GetLastCloudState();
    const bool festival_active = !world_state.GetActiveFestivalId().empty();
    const auto player_position = world_state.GetPlayer().GetPosition();
    const float far_distance_sq = kOffscreenDistanceThreshold * kOffscreenDistanceThreshold;

    for (auto& npc : world_state.MutableNpcs()) {
        npc.state = NpcState::Patrol;
        float npc_delta_seconds = delta_seconds;

        const float dx = npc.position.x - player_position.x;
        const float dy = npc.position.y - player_position.y;
        const bool is_far_from_player = (dx * dx + dy * dy) > far_distance_sq;
        if (is_far_from_player) {
            float& acc = offscreen_update_accumulator_[npc.id];
            acc += std::max(0.0f, delta_seconds);
            if (acc < kOffscreenUpdateIntervalSeconds) {
                continue;
            }
            npc_delta_seconds = acc;
            acc = 0.0f;
        } else {
            offscreen_update_accumulator_[npc.id] = 0.0f;
        }

        UpdateNpcPosition_(npc, world_state.GetClock().Day(),
                           current_minute, world_state.GetClock().Season(), cloud_state, festival_active, npc_delta_seconds);
    }
}

// ============================================================================
// 【NpcScheduleSystem::GetVisibleNpcLocations】获取可见 NPC 位置
// ============================================================================
std::vector<NpcLocationEntry> NpcScheduleSystem::GetVisibleNpcLocations(
    const std::vector<NpcActor>& npcs,
    int min_heart_level_to_show
) const {
    std::vector<NpcLocationEntry> result;

    for (const auto& npc : npcs) {
        if (!npc.visible) continue;
        if (npc.heart_level < min_heart_level_to_show) continue;

        NpcLocationEntry entry;
        entry.npc_id = npc.id;
        entry.npc_name = npc.display_name;
        entry.location_name = npc.current_location;
        entry.activity = npc.current_activity;
        entry.heart_level = npc.heart_level;
        entry.is_visible = true;
        result.push_back(entry);
    }

    // 按好感度排序（高好感度在前）
    std::sort(result.begin(), result.end(), [](const NpcLocationEntry& a, const NpcLocationEntry& b) {
        return a.heart_level > b.heart_level;
    });

    return result;
}

}  // namespace CloudSeamanor::engine
