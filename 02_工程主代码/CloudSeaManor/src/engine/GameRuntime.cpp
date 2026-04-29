#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/app/CloudSeaManor.hpp"
#include "CloudSeamanor/app/GameAppFarming.hpp"
#include "CloudSeamanor/app/GameAppHud.hpp"
#include "CloudSeamanor/app/GameAppNpc.hpp"
#include "CloudSeamanor/app/GameAppSave.hpp"
#include "CloudSeamanor/app/GameAppScene.hpp"
#include "CloudSeamanor/app/GameAppSpiritBeast.hpp"
#include "CloudSeamanor/engine/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/engine/TargetHintRuntime.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/engine/EventBus.hpp"
#include "CloudSeamanor/domain/FestivalGameplayMvp.hpp"
#include "CloudSeamanor/domain/HungerTable.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/engine/FarmingLogic.hpp"
#include "CloudSeamanor/Profiling.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/domain/BuffSystem.hpp"
#include "CloudSeamanor/domain/DiarySystem.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"
#include "CloudSeamanor/domain/TeaBushTable.hpp"
#include "CloudSeamanor/domain/TeaTable.hpp"

#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"
#include "CloudSeamanor/engine/systems/TutorialSystem.hpp"
#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"
#include "CloudSeamanor/engine/BattleUI.hpp"

#include <algorithm>
#include <cmath>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {

std::vector<std::string> SplitSaveFields_(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : line) {
        if (ch == '|') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    out.push_back(cur);
    return out;
}

std::uint32_t ComputeSaveChecksum_(const std::string& text) {
    std::uint32_t hash = 2166136261u;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

std::string BuildChecksumPayload_(const std::vector<std::string>& lines, std::size_t start) {
    std::ostringstream oss;
    for (std::size_t i = start; i < lines.size(); ++i) {
        oss << lines[i] << '\n';
    }
    return oss.str();
}

std::string BuildBattleOutcomeSummary_(
    const CloudSeamanor::engine::BattleResult& result,
    const bool exit_by_retreat) {
    std::unordered_map<std::string, int> reward_count;
    for (const auto& item_id : result.items_gained) {
        if (!item_id.empty()) {
            reward_count[item_id] += 1;
        }
    }
    if (result.spirits_purified > 0) {
        reward_count["spirit_dust"] += result.spirits_purified;
    }

    int total_item_count = 0;
    for (const auto& [_, count] : reward_count) {
        if (count > 0) {
            total_item_count += count;
        }
    }

    std::vector<std::pair<std::string, int>> reward_items;
    reward_items.reserve(reward_count.size());
    for (const auto& [item_id, count] : reward_count) {
        if (!item_id.empty() && count > 0) {
            reward_items.emplace_back(item_id, count);
        }
    }
    std::sort(reward_items.begin(), reward_items.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });

    std::string top_drops = "无";
    if (!reward_items.empty()) {
        top_drops.clear();
        const std::size_t summary_count = std::min<std::size_t>(3, reward_items.size());
        for (std::size_t i = 0; i < summary_count; ++i) {
            if (i > 0) {
                top_drops += "、";
            }
            top_drops += reward_items[i].first + "x" + std::to_string(reward_items[i].second);
        }
    }

    std::string outcome = "净化未完成";
    if (exit_by_retreat) {
        outcome = "主动撤退";
    } else if (result.victory) {
        outcome = "净化成功";
    }
    return "【战斗结算】" + outcome
        + "  目标 " + std::to_string(result.spirits_purified) + "/" + std::to_string(result.spirits_total)
        + "  经验 +" + std::to_string(static_cast<int>(result.total_exp_gained))
        + "  物品 +" + std::to_string(total_item_count)
        + "  掉落: " + top_drops;
}

bool ContainsTokenCaseInsensitive_(const std::string& haystack, const std::string& token) {
    if (token.empty() || haystack.empty()) {
        return false;
    }
    auto lower = [](unsigned char ch) -> char {
        return static_cast<char>(std::tolower(ch));
    };
    std::string h;
    h.reserve(haystack.size());
    for (const char ch : haystack) {
        h.push_back(lower(static_cast<unsigned char>(ch)));
    }
    std::string t;
    t.reserve(token.size());
    for (const char ch : token) {
        t.push_back(lower(static_cast<unsigned char>(ch)));
    }
    return h.find(t) != std::string::npos;
}

bool IsGenericBattleAnchor_(const CloudSeamanor::domain::Interactable& object) {
    // 优先：显式 enemy_id 永远视作战斗锚点。
    if (!object.EnemyId().empty()) {
        return true;
    }
    const std::string& label = object.Label();
    static const std::array<const char*, 8> kTokens{
        "spirit beast",
        "spirit zone",
        "battle",
        "enemy",
        "boss",
        "encounter",
        "polluted",
        "combat"
    };
    for (const char* token : kTokens) {
        if (ContainsTokenCaseInsensitive_(label, token)) {
            return true;
        }
    }
    // 兜底：灵界采集点/工作台不判定为战斗锚点，避免误触。
    return false;
}

std::string CatchFishForSeason_(const CloudSeamanor::domain::GameClock& clock, int attempts) {
    switch (clock.Season()) {
    case CloudSeamanor::domain::Season::Spring:
        return (attempts % 2 == 0) ? "mist_carp" : "tea_shrimp";
    case CloudSeamanor::domain::Season::Summer:
        return (clock.Hour() >= 18) ? "tide_eel" : "cloud_koi";
    case CloudSeamanor::domain::Season::Autumn:
        return (attempts % 3 == 0) ? "moon_silverfish" : "mist_carp";
    case CloudSeamanor::domain::Season::Winter:
        return (clock.Hour() < 12) ? "cloud_koi" : "moon_silverfish";
    }
    return "mist_carp";
}

void EmitRetentionEvent_(const std::string& event_id, const std::string& payload) {
    CloudSeamanor::infrastructure::Logger::Info(
        "[RETENTION] " + event_id + "|" + payload);
}

void RefreshSaveChecksum_(const std::filesystem::path& save_path) {
    std::ifstream in(save_path);
    if (!in.is_open()) {
        return;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    if (lines.empty()) {
        return;
    }

    std::size_t checksum_index = 0;
    const auto first = SplitSaveFields_(lines.front());
    if (first.size() >= 2 && first[0] == "version") {
        checksum_index = 1;
        if (lines.size() == 1) {
            lines.push_back("checksum|0");
        }
    }
    if (checksum_index >= lines.size()) {
        return;
    }
    const auto checksum_fields = SplitSaveFields_(lines[checksum_index]);
    if (checksum_fields.empty() || checksum_fields[0] != "checksum") {
        lines.insert(lines.begin() + static_cast<long long>(checksum_index), "checksum|0");
    }
    const std::uint32_t checksum = ComputeSaveChecksum_(BuildChecksumPayload_(lines, checksum_index + 1));
    lines[checksum_index] = "checksum|" + std::to_string(checksum);

    std::ofstream out(save_path, std::ios::trunc);
    if (!out.is_open()) {
        return;
    }
    for (const auto& ln : lines) {
        out << ln << '\n';
    }
}

void ApplyFestivalAutoRewardSpec_(
    CloudSeamanor::engine::GameWorldState& world_state,
    const CloudSeamanor::domain::FestivalAutoRewardSpec& spec,
    int game_day) {
    auto& fr = world_state.MutableFestivalRuntime();
    world_state.MutableGold() += spec.gold;
    if (spec.stamina > 0) {
        const float hunger_mul = world_state.GetHunger().StaminaRecoveryMultiplier();
        const float buff_mul = world_state.GetBuffs().StaminaRecoveryMultiplier();
        world_state.MutableStamina().Recover(static_cast<float>(spec.stamina) * hunger_mul * buff_mul);
    }
    for (const auto& [item_id, count] : spec.grant_items) {
        if (count > 0) {
            world_state.MutableInventory().AddItem(item_id, count);
        }
    }
    if (spec.favor_all_npcs > 0) {
        for (auto& npc : world_state.MutableNpcs()) {
            npc.favor += spec.favor_all_npcs;
            npc.heart_level = CloudSeamanor::engine::NpcHeartLevelFromFavor(npc.favor);
        }
    }
    if (spec.set_qingming_double_spirit) {
        fr.qingming_double_spirit_gather_day = game_day;
    }
    if (spec.set_flower_bloom_visual) {
        fr.flower_bloom_visual_day = game_day;
    }
    if (spec.mid_autumn_regen_bonus_until_day >= 0) {
        fr.mid_autumn_regen_bonus_until_day = spec.mid_autumn_regen_bonus_until_day;
    }
    if (spec.set_double_ninth_chrysanthemum) {
        fr.double_ninth_chrysanthemum_day = game_day;
    }
    if (spec.set_harvest_sell_bonus) {
        fr.harvest_festival_sell_bonus_day = game_day;
    }
    if (spec.set_tea_culture_contest) {
        fr.tea_culture_contest_day = game_day;
    }
    if (spec.set_winter_polar_night) {
        fr.winter_solstice_polar_anchor_day = game_day;
    }
}

void SyncWorkshopByMainHouseLevel_(
    const int main_house_level,
    CloudSeamanor::domain::WorkshopSystem& workshop) {
    int target_level = 1;
    int target_slots = 1;
    for (int level = 1;
         level < std::min(main_house_level, CloudSeamanor::engine::kMainHouseMaxLevel);
         ++level) {
        const auto upgrade = CloudSeamanor::engine::QueryMainHouseUpgradeCost(level);
        if (upgrade.workshop_level > 0) {
            target_level = std::max(target_level, upgrade.workshop_level);
        }
        if (upgrade.workshop_slots > 0) {
            target_slots = std::max(target_slots, upgrade.workshop_slots);
        }
    }
    (void)workshop.Upgrade(target_level, target_slots);
}

void SyncUnlockedRecipesToWorkshop_(
    const std::unordered_map<std::string, bool>& unlocks,
    CloudSeamanor::domain::WorkshopSystem& workshop) {
    workshop.ResetUnlockedRecipes();
    for (const auto& [id, unlocked] : unlocks) {
        if (unlocked) {
            workshop.UnlockRecipe(id);
        }
    }
}

void TryUnlockDiaryEntries_(CloudSeamanor::engine::GameWorldState& world_state,
                            const std::string& active_festival_id,
                            const std::function<void(const std::string&, float)>& push_hint) {
    CloudSeamanor::domain::DiarySystem diary_system;
    const int day = world_state.GetClock().Day();
    auto& entries = world_state.MutableDiaryEntries();
    auto unlock = [&](const std::string& id) {
        if (const auto* def = diary_system.FindById(id);
            def && diary_system.UnlockOnce(*def, day, entries)) {
            push_hint("【庄园日记】新增条目：" + def->title, 2.8f);
        }
    };

    if (world_state.GetInventory().CountOf("TeaLeaf") > 0 || world_state.GetInventory().CountOf("TeaPack") > 0) {
        unlock("harvest_first");
    }
    if (world_state.GetPurifyReturnDays() > 0) {
        unlock("purify_return");
    }
    if (!active_festival_id.empty()) {
        unlock("festival_memory");
    }
    const bool bonded = std::any_of(world_state.GetNpcs().begin(), world_state.GetNpcs().end(), [](const CloudSeamanor::engine::NpcActor& npc) {
        return npc.heart_level >= 4;
    });
    if (bonded) {
        unlock("favor_bond");
    }
    if (entries.size() >= 4 && world_state.GetInventory().CountOf("heirloom_album") <= 0) {
        world_state.MutableInventory().AddItem("heirloom_album", 1);
        push_hint("【庄园日记】你集齐了首批日记，获得纪念道具：传承册。", 3.0f);
    }
}

std::string SkillBranchIdFor_(CloudSeamanor::domain::SkillType skill) {
    using CloudSeamanor::domain::SkillType;
    switch (skill) {
    case SkillType::SpiritFarm: return "abundance";
    case SkillType::SpiritForage: return "swiftstep";
    case SkillType::SpiritFish: return "tidewatch";
    case SkillType::SpiritMine: return "refine";
    case SkillType::SpiritGuard: return "ward";
    }
    return "abundance";
}

std::string SkillBranchAltIdFor_(CloudSeamanor::domain::SkillType skill) {
    using CloudSeamanor::domain::SkillType;
    switch (skill) {
    case SkillType::SpiritFarm: return "conservation";
    case SkillType::SpiritForage: return "pathfinder";
    case SkillType::SpiritFish: return "deepcurrent";
    case SkillType::SpiritMine: return "prospect";
    case SkillType::SpiritGuard: return "barrier";
    }
    return "conservation";
}

void EnsureSkillBranchesUnlocked_(CloudSeamanor::engine::GameWorldState& world_state,
                                  CloudSeamanor::domain::SkillSystem& skills,
                                  const std::function<void(const std::string&, float)>& push_hint) {
    using CloudSeamanor::domain::SkillType;
    const SkillType all_skills[] = {
        SkillType::SpiritFarm, SkillType::SpiritForage, SkillType::SpiritFish,
        SkillType::SpiritMine, SkillType::SpiritGuard};
    for (const auto skill : all_skills) {
        if (skills.GetLevel(skill) < 10) {
            continue;
        }
        const std::string key = CloudSeamanor::domain::SkillTypeToString(skill);
        auto& branches = world_state.MutableSkillBranches();
        auto& pending = world_state.MutablePendingSkillBranches();
        const bool already_pending = std::find(pending.begin(), pending.end(), key) != pending.end();
        if (branches.count(key) == 0 && !already_pending) {
            pending.push_back(key);
            push_hint(
                "【分支觉醒】" + key + " 达到 Lv.10。去装饰台选择 A 分支，或去客栈柜台选择 B 分支。",
                3.2f);
        }
    }
}

std::string SkillBranchIdFor_(const std::string& skill_name) {
    if (skill_name == "灵农") return "abundance";
    if (skill_name == "灵觅") return "swiftstep";
    if (skill_name == "灵钓") return "tidewatch";
    if (skill_name == "灵矿") return "refine";
    if (skill_name == "灵卫") return "ward";
    return "branch_a";
}

std::string SkillBranchAltIdFor_(const std::string& skill_name) {
    if (skill_name == "灵农") return "conservation";
    if (skill_name == "灵觅") return "pathfinder";
    if (skill_name == "灵钓") return "deepcurrent";
    if (skill_name == "灵矿") return "prospect";
    if (skill_name == "灵卫") return "barrier";
    return "branch_b";
}

}  // namespace

namespace CloudSeamanor::engine {

namespace {

struct DiaryRegistryRowLocal {
    std::string id;
    std::string title;
    std::string summary;
};

struct FestivalRegistryRowLocal {
    std::string id;
    std::string name;
    std::string season;
    int day = 1;
};

struct ToolRegistryRowLocal {
    std::string id;
    std::string tool_type;
    std::string tier;
};

} // namespace

// RE-207 transitional aliases: runtime internals grouped into `state_` + `services_`.
#define config_ state_.config
#define time_scale_ state_.time_scale
#define save_path_ state_.save_path
#define active_save_slot_ state_.active_save_slot
#define last_reset_day_ state_.last_reset_day
#define npc_text_mappings_ world_state_.MutableNpcTextMappings()
#define last_contract_completed_count_ state_.last_contract_completed_count
#define current_bgm_path_ state_.current_bgm_path
#define in_battle_mode_ state_.in_battle_mode
#define dialogue_data_root_ state_.dialogue_data_root
#define map_root_override_ state_.map_root_override
#define last_cloud_state_ world_state_.MutableLastCloudState()

#define tmx_map_ services_.tmx_map
#define player_movement_ services_.player_movement
#define crop_growth_ services_.crop_growth
#define achievement_system_ services_.achievement_system
#define npc_schedule_ services_.npc_schedule
#define quest_manager_ services_.quest_manager
#define npc_delivery_ services_.npc_delivery
#define inn_system_ services_.inn_system
#define coop_system_ services_.coop_system
#define barn_system_ services_.barn_system
#define pet_system_ services_.pet_system
#define spirit_beast_ services_.spirit_beast
#define spirit_realm_manager_ services_.spirit_realm_manager
#define tea_garden_ services_.tea_garden
#define modules_ services_.modules
#define dialogue_manager_ services_.dialogue_manager
#define plot_system_ services_.plot_system
#define mod_loader_ services_.mod_loader
#define battle_manager_ services_.battle_manager

GameRuntime::GameRuntime() = default;

void GameRuntime::RecoverStaminaScaled_(const float amount) {
    const float clamped = std::max(0.0f, amount);
    if (clamped <= 0.0f) {
        return;
    }
    const float hunger_mul = world_state_.GetHunger().StaminaRecoveryMultiplier();
    const float buff_mul = world_state_.GetBuffs().StaminaRecoveryMultiplier();
    world_state_.MutableStamina().Recover(clamped * hunger_mul * buff_mul);
}

bool GameRuntime::ConsumeStaminaScaled_(const float amount) {
    const float clamped = std::max(0.0f, amount);
    if (clamped <= 0.0f) {
        return true;
    }
    const float hunger_mul = world_state_.GetHunger().StaminaCostMultiplier();
    const float buff_mul = world_state_.GetBuffs().StaminaCostMultiplier();
    float scaled = clamped * hunger_mul * buff_mul;
    const auto& branches = world_state_.GetSkillBranches();
    const auto it = branches.find("灵农");
    if (it != branches.end() && it->second == "abundance") {
        scaled *= 0.95f;
    } else if (it != branches.end() && it->second == "conservation") {
        scaled *= 0.92f;
    }
    if (world_state_.MutableStamina().Current() < scaled) {
        return false;
    }
    world_state_.MutableStamina().Consume(scaled);
    return true;
}

std::string GameRuntime::InferWeaponFromInventory_(const CloudSeamanor::domain::Inventory& inv) {
    for (const auto& slot : inv.Slots()) {
        if (slot.count <= 0) {
            continue;
        }
        if (slot.item_id.rfind("weapon_", 0) == 0) {
            return slot.item_id;
        }
    }
    return {};
}

void GameRuntime::SyncEquippedWeaponFromSaveAndInventory_() {
    if (state_.equipped_weapon_id.empty()) {
        state_.equipped_weapon_id = InferWeaponFromInventory_(world_state_.GetInventory());
    }
    if (!state_.equipped_weapon_id.empty()) {
        (void)battle_manager_.SetEquippedWeapon(state_.equipped_weapon_id);
    }
}

bool GameRuntime::HasQuestSkill_(const std::string& id) const {
    return state_.unlocked_quest_skills.contains(id);
}

void GameRuntime::SyncQuestSkills_() {
    std::vector<std::string> skills;
    skills.reserve(state_.unlocked_quest_skills.size());
    for (const auto& id : state_.unlocked_quest_skills) {
        skills.push_back(id);
    }
    battle_manager_.SetQuestSkills(skills);
}

void GameRuntime::ApplyQuestSkillExploreEffects_(float delta_seconds) {
    // Explore quest skill 1 (passive-ish MVP): qs_wind_step
    // Unlocked effect: +10% move speed always; plus a light "burst" after interaction.
    const bool have_wind_step = HasQuestSkill_("qs_wind_step");
    const float base_speed = config_.GetFloat("player_speed", GameConstants::Player::Speed);
    float speed_mul = 1.0f;
    if (have_wind_step) {
        speed_mul *= 1.10f;
    }
    if (state_.quest_wind_step_remaining_seconds > 0.0f) {
        state_.quest_wind_step_remaining_seconds = std::max(
            0.0f, state_.quest_wind_step_remaining_seconds - delta_seconds);
        speed_mul *= 1.60f;
    }
    world_state_.MutableConfig().player_speed = base_speed * speed_mul;
}

namespace {
std::string SeasonalBgmPath_(CloudSeamanor::domain::Season season) {
    using CloudSeamanor::domain::Season;
    switch (season) {
    case Season::Spring: return "assets/audio/bgm/spring_theme.wav";
    case Season::Summer: return "assets/audio/bgm/summer_theme.wav";
    case Season::Autumn: return "assets/audio/bgm/autumn_theme.wav";
    case Season::Winter: return "assets/audio/bgm/winter_theme.wav";
    }
    return "assets/audio/bgm/spring_theme.wav";
}
} // namespace

namespace {
std::vector<PriceTableEntry> LoadPriceTable_(const std::string& path) {
    std::vector<PriceTableEntry> entries;
    std::ifstream in(path);
    if (!in.is_open()) {
        return entries;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return entries;
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string f;
        PriceTableEntry e;
        int col = 0;
        while (std::getline(ss, f, ',')) {
            switch (col) {
            case 0: e.item_id = f; break;
            case 1: e.buy_price = std::max(0, std::stoi(f)); break;
            case 2: e.sell_price = std::max(0, std::stoi(f)); break;
            case 3: e.buy_from = f; break;
            case 4: e.category = f; break;
            default: break;
            }
            ++col;
        }
        if (!e.item_id.empty()) entries.push_back(std::move(e));
    }
    return entries;
}

std::vector<PriceTableEntry> LoadPriceTableFromRoots_(
    const std::vector<std::filesystem::path>& data_roots) {
    std::unordered_map<std::string, PriceTableEntry> merged;
    for (const auto& root : data_roots) {
        const auto path = root / "PriceTable.csv";
        const auto entries = LoadPriceTable_(path.string());
        for (const auto& entry : entries) {
            merged[entry.item_id] = entry;
        }
    }
    std::vector<PriceTableEntry> out;
    out.reserve(merged.size());
    for (const auto& kv : merged) {
        out.push_back(kv.second);
    }
    return out;
}

std::vector<MailTriggerRule> LoadMailTriggerRules_(const std::string& path) {
    std::vector<MailTriggerRule> rules;
    std::ifstream in(path);
    if (!in.is_open()) {
        CloudSeamanor::infrastructure::Logger::Warning(
            "GameRuntime: MailRuleTable.csv 未找到，跳过规则触发。path=" + path);
        return rules;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return rules;
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string f;
        std::vector<std::string> fields;
        while (std::getline(ss, f, ',')) {
            fields.push_back(f);
        }
        if (fields.size() < 8) continue;
        try {
            MailTriggerRule r;
            r.id = fields[0];
            r.trigger_type = fields[1];
            r.trigger_arg = fields[2];
            r.item_id = fields[3];
            r.count = std::max(1, std::stoi(fields[4]));
            r.delay_days = std::max(0, std::stoi(fields[5]));
            r.hint_text = fields[6];
            r.enabled = (std::stoi(fields[7]) != 0);
            r.sender_template = (fields.size() >= 9) ? fields[8] : "";
            r.subject_template = (fields.size() >= 10) ? fields[9] : "";
            r.body_template = (fields.size() >= 11) ? fields[10] : "";
            r.cooldown_policy = (fields.size() >= 12) ? fields[11] : "daily";
            if (!r.id.empty() && !r.item_id.empty()) {
                rules.push_back(std::move(r));
            }
        } catch (...) {
            continue;
        }
    }
    CloudSeamanor::infrastructure::Logger::Info(
        "GameRuntime: 邮件触发规则加载完成 count=" + std::to_string(rules.size()));
    return rules;
}

std::string ResolveDataFileFromRoots_(
    const std::vector<std::filesystem::path>& data_roots,
    const std::string& relative_file,
    const std::string& fallback_path) {
    std::string selected = fallback_path;
    for (const auto& root : data_roots) {
        const auto candidate = root / relative_file;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            selected = candidate.string();
        }
    }
    return selected;
}

std::string ResolveMapFromMods_(
    const CloudSeamanor::engine::ModLoader& mod_loader,
    const std::string& requested_map_path) {
    std::string resolved_map_path = requested_map_path;
    std::error_code ec;
    for (const auto& mod : mod_loader.LoadedMods()) {
        const auto mod_map = std::filesystem::path("mods")
            / mod.id
            / "maps"
            / std::filesystem::path(requested_map_path).filename();
        if (std::filesystem::exists(mod_map, ec) && std::filesystem::is_regular_file(mod_map, ec)) {
            resolved_map_path = mod_map.string();
        }
    }
    return resolved_map_path;
}

std::string CurrentSeasonTag_(CloudSeamanor::domain::Season season) {
    using CloudSeamanor::domain::Season;
    switch (season) {
    case Season::Spring: return "spring";
    case Season::Summer: return "summer";
    case Season::Autumn: return "autumn";
    case Season::Winter: return "winter";
    }
    return "spring";
}

struct NpcRelationshipEventRowLocal {
    std::string id;
    std::string type;
    std::string npc_a;
    std::string npc_b;
    std::string trigger_season;
    int result_relationship_value = 0;
    std::string shared_schedule_tag;
    std::string mail_id;
};

bool SeasonMatches_(const std::string& configured, CloudSeamanor::domain::Season season) {
    if (configured.empty() || configured == "Any") {
        return true;
    }
    const std::string current_en = CurrentSeasonTag_(season);
    const std::string current_cn = CloudSeamanor::domain::GameClock::SeasonName(season);
    return ContainsTokenCaseInsensitive_(configured, current_en)
        || ContainsTokenCaseInsensitive_(configured, current_cn);
}

std::vector<NpcRelationshipEventRowLocal> LoadNpcRelationshipEvents_() {
    std::vector<NpcRelationshipEventRowLocal> rows;
    std::ifstream in("assets/data/npc/npc_relationship_events.csv");
    if (!in.is_open()) {
        return rows;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("Id,", 0) == 0) {
            continue;
        }
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            fields.push_back(part);
        }
        if (fields.size() < 9) {
            continue;
        }
        NpcRelationshipEventRowLocal row;
        row.id = fields[0];
        row.type = fields[1];
        row.npc_a = fields[2];
        row.npc_b = fields[3];
        row.trigger_season = fields[5];
        row.result_relationship_value = std::atoi(fields[6].c_str());
        row.shared_schedule_tag = fields[7];
        row.mail_id = fields[8];
        rows.push_back(std::move(row));
    }
    return rows;
}

bool ShouldTriggerMailRule_(
    const MailTriggerRule& rule,
    const CloudSeamanor::domain::GameClock& clock,
    const std::string& active_festival_id,
    const std::vector<NpcActor>& npcs) {
    try {
        if (rule.trigger_type == "day_in_season_eq") {
            return clock.DayInSeason() == std::max(1, std::stoi(rule.trigger_arg));
        }
        if (rule.trigger_type == "weekday_mod_eq") {
            const auto pos = rule.trigger_arg.find(':');
            if (pos == std::string::npos) return false;
            const int divisor = std::max(1, std::stoi(rule.trigger_arg.substr(0, pos)));
            const int remainder = std::max(0, std::stoi(rule.trigger_arg.substr(pos + 1)));
            return (clock.Day() % divisor) == remainder;
        }
        if (rule.trigger_type == "season_first_day") {
            return clock.DayInSeason() == 1
                && CurrentSeasonTag_(clock.Season()) == rule.trigger_arg;
        }
        if (rule.trigger_type == "festival_id_eq") {
            return !active_festival_id.empty() && active_festival_id == rule.trigger_arg;
        }
        if (rule.trigger_type == "npc_favor_ge") {
            const auto pos = rule.trigger_arg.find(':');
            if (pos == std::string::npos) return false;
            const std::string npc_id = rule.trigger_arg.substr(0, pos);
            const int threshold = std::stoi(rule.trigger_arg.substr(pos + 1));
            for (const auto& npc : npcs) {
                if (npc.id == npc_id) {
                    return npc.favor >= threshold;
                }
            }
            return false;
        }
        if (rule.trigger_type == "npc_heart_ge") {
            const auto pos = rule.trigger_arg.find(':');
            if (pos == std::string::npos) return false;
            const std::string npc_id = rule.trigger_arg.substr(0, pos);
            const int threshold = std::stoi(rule.trigger_arg.substr(pos + 1));
            for (const auto& npc : npcs) {
                if (npc.id == npc_id) {
                    return npc.heart_level >= threshold;
                }
            }
            return false;
        }
    } catch (...) {
        return false;
    }
    return false;
}

std::string ApplyMailTemplate_(
    const std::string& templ,
    const MailTriggerRule& rule,
    int current_day) {
    std::string out = templ;
    auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
        std::string::size_type pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };
    replace_all(out, "$[RULE_ID]", rule.id);
    replace_all(out, "$[ITEM_ID]", rule.item_id);
    replace_all(out, "$[COUNT]", std::to_string(rule.count));
    replace_all(out, "$[DAY]", std::to_string(current_day));
    return out;
}

int CurrentSeasonKey_(const CloudSeamanor::domain::GameClock& clock) {
    return clock.Year() * 10 + static_cast<int>(clock.Season());
}

bool IsMailRuleAllowedByCooldown_(
    const MailTriggerRule& rule,
    const CloudSeamanor::domain::GameClock& clock,
    const std::unordered_map<std::string, int>& last_day,
    const std::unordered_map<std::string, int>& last_season_key,
    const std::unordered_map<std::string, bool>& triggered_once) {
    const auto policy = rule.cooldown_policy.empty() ? std::string("daily") : rule.cooldown_policy;
    if (policy == "once") {
        const auto it = triggered_once.find(rule.id);
        return it == triggered_once.end() || !it->second;
    }
    if (policy == "season") {
        const int current_key = CurrentSeasonKey_(clock);
        const auto it = last_season_key.find(rule.id);
        return it == last_season_key.end() || it->second != current_key;
    }
    // 默认按 daily 防刷屏。
    const auto it = last_day.find(rule.id);
    return it == last_day.end() || it->second != clock.Day();
}
}

// ============================================================================
// 【GameRuntime::Initialize】初始化
// ============================================================================
void GameRuntime::Initialize(
    const std::string& config_path,
    const std::string& schedule_path,
    const std::string& gift_path,
    const std::string& npc_text_path,
    const std::string& map_path,
    const GameRuntimeCallbacks& callbacks
) {
    callbacks_ = callbacks;
    if (!mod_loader_.LoadAll("mods")) {
        CloudSeamanor::infrastructure::Logger::Warning("ModLoader: no valid mods loaded from mods/");
    } else {
        CloudSeamanor::infrastructure::Logger::Info("ModLoader: mods loaded successfully.");
    }
    world_state_.MutableModHooks() = mod_loader_.Hooks();
    const auto data_roots = mod_loader_.BuildDataRoots("assets/data");
    if (resource_manager_ != nullptr) {
        resource_manager_->SetDataRoots(data_roots);
    }
    data_registry_ = {};
    data_registry_.AttachResourceManager(resource_manager_);
    data_registry_.SetDataRoots(data_roots);
    data_registry_.RegisterPath("crops", "assets/data");
    data_registry_.RegisterPath("weapons", "assets/data");
    data_registry_.RegisterPath("npcs", "assets/data");
    data_registry_.RegisterPath("recipes", "assets/data/recipes");
    data_registry_.RegisterPath("fishing", "assets/data/fishing");
    data_registry_.RegisterPath("festival", "assets/data/festival");
    data_registry_.RegisterPath("diary", "assets/data/diary");
    data_registry_.RegisterPath("skills", "assets/data/skills");
    static CloudSeamanor::infrastructure::DataTable<DiaryRegistryRowLocal> diary_registry_table{
        [](const DiaryRegistryRowLocal& row) { return row.id; }
    };
    static CloudSeamanor::infrastructure::DataTable<FestivalRegistryRowLocal> festival_registry_table{
        [](const FestivalRegistryRowLocal& row) { return row.id; }
    };
    static CloudSeamanor::infrastructure::DataTable<ToolRegistryRowLocal> tool_registry_table{
        [](const ToolRegistryRowLocal& row) { return row.id; }
    };
    data_registry_.RegisterCsvTable<DiaryRegistryRowLocal>(
        "diary_entries",
        "diary/entries.csv",
        &diary_registry_table,
        [](const CloudSeamanor::infrastructure::CsvRow& row,
           std::vector<CloudSeamanor::infrastructure::DataIssue>& issues)
            -> std::optional<DiaryRegistryRowLocal> {
            DiaryRegistryRowLocal parsed{
                .id = row.Get("id"),
                .title = row.Get("title"),
                .summary = row.Get("summary"),
            };
            if (parsed.id.empty() || parsed.title.empty()) {
                issues.push_back({
                    .severity = CloudSeamanor::infrastructure::DataIssueSeverity::Error,
                    .line_number = row.line_number,
                    .message = "日记表缺少必填字段 id/title",
                });
                return std::nullopt;
            }
            return parsed;
        });
    data_registry_.RegisterCsvTable<FestivalRegistryRowLocal>(
        "festival_definitions",
        "festival/festival_definitions.csv",
        &festival_registry_table,
        [](const CloudSeamanor::infrastructure::CsvRow& row,
           std::vector<CloudSeamanor::infrastructure::DataIssue>& issues)
            -> std::optional<FestivalRegistryRowLocal> {
            const std::string id = row.Has("id") ? row.Get("id") : row.Get("col0");
            const std::string name = row.Has("name") ? row.Get("name") : row.Get("col1");
            const std::string season = row.Has("season") ? row.Get("season") : row.Get("col2");
            const int day = row.Has("day") ? row.GetInt("day", 0) : row.GetInt("col3", 0);
            FestivalRegistryRowLocal parsed{.id = id, .name = name, .season = season, .day = day};
            if (parsed.id.empty() || parsed.name.empty() || parsed.day <= 0) {
                issues.push_back({
                    .severity = CloudSeamanor::infrastructure::DataIssueSeverity::Warning,
                    .line_number = row.line_number,
                    .message = "节日表存在缺失字段，已跳过该行",
                });
                return std::nullopt;
            }
            return parsed;
        });
    data_registry_.RegisterCsvTable<ToolRegistryRowLocal>(
        "tool_definitions",
        "tool/tool_data.csv",
        &tool_registry_table,
        [](const CloudSeamanor::infrastructure::CsvRow& row,
           std::vector<CloudSeamanor::infrastructure::DataIssue>& issues)
            -> std::optional<ToolRegistryRowLocal> {
            ToolRegistryRowLocal parsed{
                .id = row.Get("Id"),
                .tool_type = row.Get("ToolType"),
                .tier = row.Get("Tier"),
            };
            if (parsed.id.empty() || parsed.tool_type.empty() || parsed.tier.empty()) {
                issues.push_back({
                    .severity = CloudSeamanor::infrastructure::DataIssueSeverity::Warning,
                    .line_number = row.line_number,
                    .message = "工具表存在缺失字段，已跳过该行",
                });
                return std::nullopt;
            }
            return parsed;
        });
    for (const auto& entry : data_registry_.Entries()) {
        CloudSeamanor::infrastructure::Logger::Info(
            "DataRegistry: " + entry.category + " -> " + entry.path);
    }
    (void)data_registry_.LoadAll();
    for (const auto& log_line : data_registry_.ValidateAndDescribe()) {
        CloudSeamanor::infrastructure::Logger::Info("DataRegistryCheck: " + log_line);
    }
    const std::string resolved_schedule_path = ResolveDataFileFromRoots_(
        data_roots, "Schedule_Data.csv", schedule_path);
    const std::string resolved_gift_path = ResolveDataFileFromRoots_(
        data_roots, "Gift_Preference.json", gift_path);
    const std::string resolved_npc_text_path = ResolveDataFileFromRoots_(
        data_roots, "NPC_Texts.json", npc_text_path);
    const std::string resolved_npc_data_path = ResolveDataFileFromRoots_(
        data_roots, "npc/npc_data.csv", "assets/data/npc/npc_data.csv");
    const std::string resolved_quest_path = ResolveDataFileFromRoots_(
        data_roots, "QuestTable.csv", "assets/data/QuestTable.csv");
    const std::string resolved_delivery_path = ResolveDataFileFromRoots_(
        data_roots, "NpcDeliveryTable.csv", "assets/data/NpcDeliveryTable.csv");
    const std::string resolved_mail_rule_path = ResolveDataFileFromRoots_(
        data_roots, "MailRuleTable.csv", "assets/data/MailRuleTable.csv");
    const std::string resolved_crop_table_path = ResolveDataFileFromRoots_(
        data_roots, "CropTable.csv", "assets/data/CropTable.csv");
    const std::string resolved_tea_table_path = ResolveDataFileFromRoots_(
        data_roots, "TeaTable.csv", "assets/data/TeaTable.csv");
    const std::string resolved_hunger_table_path = ResolveDataFileFromRoots_(
        data_roots, "HungerTable.csv", "assets/data/HungerTable.csv");
    const std::string resolved_tea_bush_table_path = ResolveDataFileFromRoots_(
        data_roots, "TeaBushTable.csv", "assets/data/TeaBushTable.csv");
    const std::string resolved_festival_path = ResolveDataFileFromRoots_(
        data_roots, "festival/festival_definitions.csv", "assets/data/festival/festival_definitions.csv");
    std::string resolved_map_path = ResolveMapFromMods_(mod_loader_, map_path);
    dialogue_data_root_ = std::filesystem::path(resolved_schedule_path).parent_path().string();
    if (dialogue_data_root_.empty()) {
        dialogue_data_root_ = "assets/data";
    }

    // 日志初始化
    const bool config_loaded = config_.LoadFromFile(config_path);
    const std::uintmax_t log_max_kb =
        static_cast<std::uintmax_t>(config_.GetFloat("log_max_kb", 512.0f));
    CloudSeamanor::infrastructure::Logger::Initialize("logs", "CloudSeamanor.log", log_max_kb * 1024);
    CloudSeamanor::infrastructure::Logger::Info(
        config_loaded ? "已加载 gameplay 配置。" : "未找到 gameplay 配置，使用默认值。");
    if (!config_loaded) {
        CloudSeamanor::infrastructure::Logger::LogConfigLoadFailure(config_path);
    }
    const int dialogue_typing_speed_ms = static_cast<int>(
        config_.GetFloat("dialogue_typing_speed_ms", 45.0f));

    // 验证数据资产
    ValidateDataAsset(resolved_schedule_path, "Schedule_Data.csv",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);
    (void)npc_schedule_.LoadSchedule(resolved_schedule_path);
    ValidateDataAsset(resolved_gift_path, "Gift_Preference.json",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_npc_text_path, "NPC_Texts.json",
        &CloudSeamanor::infrastructure::Logger::Error,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_npc_data_path, "npc_data.csv",
        &CloudSeamanor::infrastructure::Logger::Warning,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_mail_rule_path, "MailRuleTable.csv",
        &CloudSeamanor::infrastructure::Logger::Warning,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_tea_table_path, "TeaTable.csv",
        &CloudSeamanor::infrastructure::Logger::Warning,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_hunger_table_path, "HungerTable.csv",
        &CloudSeamanor::infrastructure::Logger::Warning,
        &CloudSeamanor::infrastructure::Logger::Info);
    ValidateDataAsset(resolved_tea_bush_table_path, "TeaBushTable.csv",
        &CloudSeamanor::infrastructure::Logger::Warning,
        &CloudSeamanor::infrastructure::Logger::Info);

    // 初始化世界配置
    WorldConfig world_config;
    world_config.player_speed = config_.GetFloat("player_speed", GameConstants::Player::Speed);
    world_config.stamina_move_per_second = config_.GetFloat("stamina_move_per_second", GameConstants::Player::StaminaMovePerSecond);
    world_config.stamina_interact_cost = config_.GetFloat("stamina_interact_cost", GameConstants::Player::StaminaInteractCost);
    world_config.stamina_recover_per_second = config_.GetFloat("stamina_recover_per_second", GameConstants::Player::StaminaRecoverPerSecond);
    time_scale_ = config_.GetFloat("time_scale", GameConstants::Runtime::DefaultTimeScale);

    world_state_.MutableConfig() = world_config;
    world_state_.MutableStamina().SetMax(config_.GetFloat("player_stamina_max", GameConstants::Player::StaminaMax));
    world_state_.MutableStamina().SetCurrent(config_.GetFloat("player_stamina", GameConstants::Player::StaminaInitial));
    world_state_.MutableHunger().Initialize(
        static_cast<int>(config_.GetFloat("player_hunger", 80.0f)),
        static_cast<int>(config_.GetFloat("player_hunger_max", 100.0f)));

    // 初始化 UI 面板
    world_state_.InitializePanels();

    // 构建场景
    BuildScene(
        tmx_map_,
        world_state_.MutableGroundTiles(),
        world_state_.MutableObstacleShapes(),
        world_state_.MutableObstacleBounds(),
        world_state_.MutableInteractables(),
        world_state_.MutablePickups(),
        world_config.world_bounds,
        resolved_map_path,
        &CloudSeamanor::infrastructure::Logger::Info,
        CloudSeamanor::infrastructure::Logger::Warning);
    map_root_override_ = std::filesystem::path(resolved_map_path).parent_path().string();
    SetCropTableDataPath(resolved_crop_table_path);
    (void)CloudSeamanor::domain::GetGlobalTeaTable().LoadFromFile(resolved_tea_table_path);
    (void)CloudSeamanor::domain::GetGlobalHungerTable().LoadFromFile(resolved_hunger_table_path);
    (void)CloudSeamanor::domain::GetGlobalTeaBushTable().LoadFromFile(resolved_tea_bush_table_path);
    InitializeTeaBushes_();
    SyncTeaBushInteractables_();
    SyncNpcHouseVariantInteractables_();

    // 初始化农业系统
    BuildTeaPlots(world_state_.MutableTeaPlots());
    for (auto& plot : world_state_.MutableTeaPlots()) {
        RefreshTeaPlotVisual(plot, false);
    }
    // B-24 茶园：独立于普通农田的长期茶树地块（显式深拷贝，后续独立更新）。
    world_state_.MutableTeaGardenPlots().clear();
    world_state_.MutableTeaGardenPlots().reserve(world_state_.MutableTeaPlots().size());
    for (const auto& base_plot : world_state_.MutableTeaPlots()) {
        TeaPlot p = base_plot;
        p.crop_name = "茶园-" + p.crop_name;
        p.growth_time = std::max(160.0f, p.growth_time * 2.0f);
        world_state_.MutableTeaGardenPlots().push_back(std::move(p));
    }

    // 初始化作物生长系统
    crop_growth_.SetHintCallback(
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });
    crop_growth_.SetFertilizerMultipliers(
        config_.GetFloat("fertilizer_basic_multiplier", 1.2f),
        config_.GetFloat("fertilizer_premium_multiplier", 1.5f));

    // 初始化灵兽
    BuildSpiritBeast(world_state_.MutableSpiritBeast(),
                    world_state_.MutableClock(),
                    world_state_.MutableSpiritBeastWateredToday(),
                    world_state_.MutableInteraction().spirit_beast_highlighted);

    // 初始化 NPC
    LoadNpcTextMappings(resolved_npc_text_path, npc_text_mappings_);
    BuildNpcs(resolved_schedule_path, resolved_gift_path, resolved_npc_data_path, npc_text_mappings_, world_state_.MutableNpcs());

    // 初始化 NPC 对话管理器（支持 mod 覆盖的数据根）
    dialogue_manager_ = NpcDialogueManager(dialogue_data_root_);
    dialogue_manager_.InitializeHeartEvents();
    for (const auto& npc : world_state_.MutableNpcs()) {
        dialogue_manager_.LoadDailyDialogue(npc.id);
    }
    dialogue_manager_.SetNpcList(&world_state_.MutableNpcs());

    // 初始化拾取系统运行时
    modules_.pickup = std::make_unique<PickupSystemRuntime>(
        systems_.GetPickups(),
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });
    modules_.pickup->Initialize();
    modules_.tutorial = std::make_unique<TutorialSystem>(
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    // 初始化系统
    systems_.InitializeContracts();
    systems_.Initialize();
    (void)systems_.MutableDynamicLife().LoadFromFile(resolved_npc_data_path);
    {
        std::vector<std::string> npc_ids;
        npc_ids.reserve(world_state_.MutableNpcs().size());
        for (const auto& npc : world_state_.MutableNpcs()) {
            npc_ids.push_back(npc.id);
        }
        systems_.MutableNpcDevelopment().Initialize(npc_ids, world_state_.MutableClock().Day());
        for (auto& npc : world_state_.MutableNpcs()) {
            if (const auto* dev = systems_.GetNpcDevelopment().GetDevelopment(npc.id)) {
                npc.development_stage = dev->current_stage;
            }
        }
    }
    systems_.GetCloud().AdvanceToNextDay(0);
    systems_.GetCloud().UpdateForecastVisibility(0, 0);
    modules_.workshop = std::make_unique<WorkshopSystemRuntime>(
        systems_,
        world_state_,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    SyncTeaMachineFromWorkshop_();
    try {
        battle_manager_.Initialize(&systems_.GetCloud());
    } catch (const std::exception& ex) {
        CloudSeamanor::infrastructure::Logger::Warning(
            std::string("Battle system init skipped due to init error: ") + ex.what());
    }
    battle_manager_.SetRewardCallbacks(BattleManager::RewardCallbacks{
        .on_item_reward = [this](const std::string& item_id, int count) {
            if (state_.tide_festival_battle_active && count > 0) {
                count += 1;
            }
            world_state_.MutableInventory().AddItem(item_id, count);
        },
        .on_exp_reward = [this](float exp) {
            if (state_.tide_festival_battle_active) {
                exp *= 1.35f;
            }
            if (!systems_.GetSkills().AddExp(
                    CloudSeamanor::domain::SkillType::SpiritForage,
                    exp,
                    systems_.GetCloud().CurrentSpiritDensity())) {
                CloudSeamanor::infrastructure::Logger::Warning(
                    "Battle reward exp grant failed for SpiritForage.");
            }
        },
        .on_partner_favor_reward = [this](const std::string&, int favor_delta) {
            world_state_.MutableSpiritBeast().favor += favor_delta;
        },
        .on_notice = [this](const std::string& message) {
            callbacks_.push_hint(message, 2.8f);
        }
    });
    SyncEquippedWeaponFromSaveAndInventory_();
    SyncQuestSkills_();

    // 初始化世界
    world_state_.InitializeWorld(world_config);
    SyncWorkshopByMainHouseLevel_(world_state_.MutableMainHouseRepair().level, systems_.GetWorkshop());
    {
        // P0-002: seed a minimal cooking set; this is data-driven and persisted via recipe_unlock tags.
        auto& unlocks = world_state_.MutableRecipeUnlocks();
        unlocks["cook_tea_egg"] = true;
        unlocks["cook_veggie_soup"] = true;
        unlocks["cook_warm_milk"] = true;
        SyncUnlockedRecipesToWorkshop_(unlocks, systems_.GetWorkshop());
    }
    world_state_.MutableHunger().SetGameClock(&world_state_.MutableClock());
    world_state_.MutableInteraction().dialogue_engine.SetTypingSpeed(
        std::clamp(dialogue_typing_speed_ms, 20, 100));
    world_state_.MutableInteraction().dialogue_text =
        "欢迎回家。在云海山庄里慢慢建立属于你的日常：种田、观云、结缘灵兽、与人来往。"
        "每日22:00可查看次日云海预报；云海和灵气共同影响作物与灵茶收益；"
        "完成云海守护者契约可获得永久增益。这里没有硬性期限，错过的内容来年也能再体验。";
    SetHintMessage(world_state_,
        "欢迎来到云海山庄。先用 WASD 移动，并观察附近的高亮提示；"
        "这里没有时间压力，可以按自己的节奏慢慢来。", GameConstants::Ui::HintDuration::Welcome);

    // 初始物品
    world_state_.MutableInventory().AddItem("TeaSeed", GameConstants::Initial::TeaSeedCount);
    world_state_.MutableInventory().AddItem("TurnipSeed", GameConstants::Initial::TurnipSeedCount);
    world_state_.MutableInventory().AddItem("Wood", GameConstants::Initial::WoodCount);
    // 工具耐久（MVP：用背包数量表示剩余使用次数）
    world_state_.MutableInventory().AddItem("ToolHoe", 60);
    world_state_.MutableInventory().AddItem("ToolAxe", 60);
    world_state_.MutableInventory().AddItem("ToolPickaxe", 60);
    world_state_.MutableInventory().AddItem("ToolSickle", 60);
    world_state_.MutableInventory().AddItem("SprinklerItem", 2);
    world_state_.MutableInventory().AddItem("FertilizerItem", 4);
    world_state_.MutablePriceTable() = LoadPriceTableFromRoots_(data_roots);
    state_.mail_trigger_rules = LoadMailTriggerRules_(resolved_mail_rule_path);
    const auto grant_achievement_reward = [this](const std::string& achievement_id) {
        if (achievement_id == "first_crop") {
            world_state_.MutableGold() += 100;
            callbacks_.push_hint("成就奖励：金币 +100", 1.8f);
        } else if (achievement_id == "ten_crops") {
            world_state_.MutableGold() += 500;
            callbacks_.push_hint("成就奖励：金币 +500", 1.8f);
        } else if (achievement_id == "gift_expert") {
            world_state_.MutableInventory().AddItem("TeaPack", 1);
            callbacks_.push_hint("成就奖励：茶包 x1", 1.8f);
        } else if (achievement_id == "master_builder") {
            world_state_.MutableInventory().AddItem("Wood", 5);
            callbacks_.push_hint("成就奖励：木材 x5", 1.8f);
        } else if (achievement_id == "home_designer") {
            world_state_.MutableGold() += 200;
            callbacks_.push_hint("成就奖励：金币 +200", 1.8f);
        } else if (achievement_id == "beast_bond") {
            world_state_.MutableInventory().AddItem("Feed", 1);
            callbacks_.push_hint("成就奖励：饲料 x1", 1.8f);
        } else if (achievement_id == "beast_bond_max") {
            world_state_.MutableGold() += 300;
            callbacks_.push_hint("成就奖励：金币 +300", 1.8f);
        } else if (achievement_id == "beast_all_types") {
            world_state_.MutableInventory().AddItem("star_fragment", 1);
            callbacks_.push_hint("成就奖励：星辰碎片 x1", 1.8f);
        } else if (achievement_id == "tide_purifier") {
            world_state_.MutableInventory().AddItem("legendary_tide", 1);
            callbacks_.push_hint("成就奖励：潮汐印记 x1", 1.8f);
        } else if (achievement_id == "fest_calendar") {
            world_state_.MutableGold() += 150;
            callbacks_.push_hint("成就奖励：金币 +150", 1.8f);
        }
    };
    (void)GlobalEventBus().Subscribe("harvest", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)GlobalEventBus().Subscribe("gift", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)GlobalEventBus().Subscribe("build", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)GlobalEventBus().Subscribe("beast_bond", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)GlobalEventBus().Subscribe("beast_type_collected", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    (void)GlobalEventBus().Subscribe("FestivalTideBossVictoryEvent", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.4f); },
            grant_achievement_reward);
        plot_system_.OnGameEvent("flag_set", {
            {"flag", "festival_tide_boss_victory"}
        });
        plot_system_.OnGameEvent("flag_set", {
            {"flag", "festival_joined_cloud_tide_ritual"}
        });
        callbacks_.push_hint("主线旗标已更新：大潮祭决战完成。", 2.4f);
    });
    (void)GlobalEventBus().Subscribe("FestivalMvpRewardEvent", [this, grant_achievement_reward](const Event& event) {
        achievement_system_.HandleEvent(
            world_state_.MutableAchievements(),
            event,
            [this](const std::string& msg) { callbacks_.push_hint(msg, 2.2f); },
            grant_achievement_reward);
    });
    if (!quest_manager_.LoadFromCsv(resolved_quest_path, world_state_.MutableRuntimeQuests())) {
        CloudSeamanor::infrastructure::Logger::LogConfigLoadFailure(
            std::string("Quest table load failed: ") + resolved_quest_path);
    } else {
        CloudSeamanor::infrastructure::Logger::Info(
            std::string("Quest table loaded: ") + resolved_quest_path);
    }
    if (!npc_delivery_.LoadFromCsv(resolved_delivery_path)) {
        CloudSeamanor::infrastructure::Logger::LogConfigLoadFailure(
            std::string("NpcDelivery table load failed: ") + resolved_delivery_path);
    } else {
        CloudSeamanor::infrastructure::Logger::Info(
            std::string("NpcDelivery table loaded: ") + resolved_delivery_path);
    }
    npc_delivery_.SetHintCallback([this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); });

    // 节日系统
    systems_.GetFestivals().Initialize(resolved_festival_path);
    systems_.GetFestivals().Update(world_state_.MutableClock().Season(),
                                      world_state_.MutableClock().DayInSeason());
    {
        const std::string fest_notice = systems_.GetFestivals().GetNoticeText();
        world_state_.SetFestivalNoticeText(fest_notice);
        const auto* today_festival = systems_.GetFestivals().GetTodayFestival();
        world_state_.SetActiveFestivalId(today_festival ? today_festival->id : "");
    }
    if (!state_.festival_event_subscribed) {
        state_.festival_event_subscription_id =
            GlobalEventBus().Subscribe("FestivalActiveEvent", [this](const Event& ev) {
                const std::string festival_id = ev.GetAs<std::string>("festival_id", "");
                const std::string festival_name = ev.GetAs<std::string>("festival_name", "节日");
                const int event_day = ev.GetAs<int>("day", world_state_.MutableClock().Day());
                if (festival_id.empty()) {
                    return;
                }
                const auto* active_festival = systems_.GetFestivals().GetTodayFestival();
                if (active_festival == nullptr || active_festival->id != festival_id) {
                    return;
                }
                auto& fr = world_state_.MutableFestivalRuntime();
                const auto claimed_it = fr.mvp_auto_reward_last_day_by_festival.find(festival_id);
                if (claimed_it != fr.mvp_auto_reward_last_day_by_festival.end()
                    && claimed_it->second == event_day) {
                    return;
                }

                const auto spec = CloudSeamanor::domain::BuildFestivalAutoRewardSpec(
                    festival_id,
                    event_day,
                    world_state_.MutableClock().Year());
                ApplyFestivalAutoRewardSpec_(world_state_, spec, event_day);
                fr.mvp_auto_reward_last_day_by_festival[festival_id] = event_day;
                systems_.GetFestivals().Participate(festival_id);

                for (const auto& line : spec.hint_lines) {
                    callbacks_.push_hint(line, 3.2f);
                }

                Event mvp_ev;
                mvp_ev.type = "FestivalMvpRewardEvent";
                mvp_ev.data["festival_id"] = festival_id;
                mvp_ev.data["day"] = std::to_string(event_day);
                GlobalEventBus().Emit(mvp_ev);

                if (festival_id == "cloud_tide_ritual" || festival_id == "cloud_tide") {
                    state_.tide_festival_battle_pending = true;
                    state_.tide_festival_battle_active = false;
                    state_.tide_festival_battle_day = event_day;
                    callbacks_.push_hint("【大潮祭】潮灵异动已出现，前往灵界并靠近灵兽区域可触发祭典战。", 3.0f);
                }
                CloudSeamanor::infrastructure::Logger::Info(
                    "FestivalActiveEvent handled: " + festival_id + " (" + festival_name + ")");
            });
        state_.festival_event_subscribed = true;
    }

    // 主线剧情系统
    plot_system_.Initialize(dialogue_data_root_);
    plot_system_.SetGameClock(&world_state_.MutableClock());
    plot_system_.SetCloudSystem(&systems_.GetCloud());
    plot_system_.SetNpcHeartGetter([this](const std::string& npc_id) -> int {
        for (const auto& npc : world_state_.MutableNpcs()) {
            if (npc.id == npc_id) {
                return npc.heart_level;
            }
        }
        return 0;
    });
    plot_system_.SetCallbacks(PlotCallbacks{
        .on_chapter_start = [this](const std::string& ch) {
            callbacks_.push_hint("章节开始：" + ch, 3.0f);
        },
        .on_chapter_complete = [this](const std::string& ch) {
            callbacks_.push_hint("章节完成：" + ch, 3.0f);
        },
        .on_plot_start = [this](const std::string& plot_id) {
            callbacks_.push_hint("剧情触发：" + plot_id, 2.0f);
        },
        .on_plot_complete = [this](const std::string& plot_id) {
            callbacks_.push_hint("剧情完成：" + plot_id, 2.0f);
        },
        .on_unlock_region = [this](const std::string& region) {
            callbacks_.push_hint("新区域解锁：" + region, 3.0f);
        },
        .on_unlock_npc = [this](const std::string& npc) {
            callbacks_.push_hint("新角色加入山庄：" + npc, 3.0f);
        },
        .on_flag_set = [](const std::string&) {},
        .on_cloud_delta = [this](int delta) {
            systems_.GetCloud().SetSpiritEnergy(
                systems_.GetCloud().SpiritEnergy() + delta);
            callbacks_.push_hint(std::string("灵气 ") + (delta >= 0 ? "+" : "") +
                                    std::to_string(delta), 2.0f);
        },
        .on_favor_delta = [](const std::string&, int) {},
        .on_route_lock = [this](const std::string& route) {
            callbacks_.push_hint("路线已锁定：" + route, 4.0f);
        },
        .on_notice = [this](const std::string& notice) {
            callbacks_.push_hint(notice, 4.0f);
        },
        .log_info = &CloudSeamanor::infrastructure::Logger::Info,
    });
    plot_system_.CheckPlotTriggers();  // 第1天自动触发序章

    CloudSeamanor::infrastructure::Logger::Info("GameRuntime 初始化完成。");
}

// ============================================================================
// 【GameRuntime::SetWindow】设置窗口
// ============================================================================
void GameRuntime::SetWindow(sf::RenderWindow* window) {
    window_ = window;
}

// ============================================================================
// 【GameRuntime::CloudMultiplier】云海倍率
// ============================================================================
float GameRuntime::CloudMultiplier() const {
    return CloudGrowthMultiplier(systems_.GetCloud().CurrentState());
}

// ============================================================================
// 【GameRuntime::OnDayChanged】日期变化
// ============================================================================
void GameRuntime::OnDayChanged() {
    // =========================================================================
    // 【Daily settlement order (regression table)】
    // 目标：任何新增“每日系统”都必须按此顺序接入，避免重复结算/漏结算。
    // 1) 状态迁移：节日运行时/关系状态机/欠损计时/灵界次数/饥饿补给/茶树刷新
    // 2) 云海结算：计算昨日行为影响 -> ApplyInfluence -> AdvanceToNextDay -> Contract unlocks
    // 3) 世界日刷新：Festivals/DynamicLife/Mail rules/地块浇水与交互标记重置/病虫害/洒水器/灵界节点
    // 4) 经营产出：委托刷新/畜棚日结/客栈日结
    // 5) Buff：装饰舒适度/节日增益
    // 6) 成就：日评估
    // 7) 经济：周统计重置/价格波动/商店进货
    // =========================================================================

    world_state_.MutableFestivalRuntime().OnBeginNewDay(world_state_.MutableClock().Day());
    {
        auto& rel = world_state_.MutableSocial().relationship;
        const int day = world_state_.MutableClock().Day();
        const bool became_married = relationship_system_.OnBeginNewDay(rel, day);
        relationship_system_.ApplyDailyMarriageBuff(rel, world_state_.MutableBuffs());

        if (became_married && !rel.target_npc_id.empty()) {
            // 同步 NPC 侧显示标记（历史兼容字段，便于 UI/对话复用）。
            for (auto& npc : world_state_.MutableNpcs()) {
                npc.married = (npc.id == rel.target_npc_id);
            }
            callbacks_.push_hint("【婚礼】你与 " + rel.target_npc_id + " 结为伴侣。婚后每日获得小幅体力增益。", 3.6f);
        }
    }
    if (state_.qi_deficit_days > 0) {
        --state_.qi_deficit_days;
        callbacks_.push_hint(
            "茶园灵气受损恢复中：剩余 " + std::to_string(state_.qi_deficit_days) + " 天。",
            2.6f);
    }

    const int spirit_realm_daily_max = std::max(1, static_cast<int>(config_.GetFloat("spirit_realm_daily_max", 5.0f)));
    world_state_.SetSpiritRealmDailyMax(spirit_realm_daily_max);
    world_state_.SetSpiritRealmDailyRemaining(spirit_realm_daily_max);
    world_state_.MutableHunger().RefillForNewDay();
    UpdateTeaBushes_();

    // --- 云海结算（统一权威入口） ---
    {
        int daily_influence = 0;
        for (const auto& plot : world_state_.MutableTeaPlots()) {
            if (plot.seeded && plot.watered) daily_influence += GameConstants::Cloud::WateredPlotInfluence;
        }
        if (!world_state_.MutableMainHouseRepair().completed) daily_influence += GameConstants::Cloud::MainHouseRepairInfluence;
        if (!world_state_.MutableSpiritBeast().daily_interacted) daily_influence += GameConstants::Cloud::NoBeastInteractionPenalty;
        if (world_state_.GetPurifyReturnDays() > 0) {
            const int bonus = std::max(0, world_state_.GetPurifyReturnSpirits()) * 2;
            if (bonus > 0) {
                daily_influence += bonus;
            }
            world_state_.MutablePurifyReturnDays() = std::max(0, world_state_.GetPurifyReturnDays() - 1);
            if (world_state_.GetPurifyReturnDays() <= 0) {
                world_state_.MutablePurifyReturnSpirits() = 0;
            }
            callbacks_.push_hint("昨日净化回流：茶园生机增强（剩余 "
                                     + std::to_string(world_state_.GetPurifyReturnDays()) + " 天）。",
                                 2.4f);
        }
        systems_.AddPlayerInfluence(daily_influence);
        systems_.GetCloud().AdvanceToNextDay(world_state_.MutableClock().Day());
        systems_.CheckContractUnlocks();
    }

    const float sprinkler_radius = config_.GetFloat("sprinkler_radius", 80.0f);
    const auto current_season = world_state_.MutableClock().Season();
    auto season_changed_event = world_state_.MutableClock().ConsumeSeasonChangedEvent();
    const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();

    systems_.UpdateDaily(current_season,
                         static_cast<int>(world_state_.GetSessionTime() / 100),
                         cloud_density);
    for (auto& npc : world_state_.MutableNpcs()) {
        const std::string previous_location = npc.current_location;
        if (systems_.MutableNpcDevelopment().TryAdvanceByDay(npc.id, world_state_.MutableClock().Day())) {
            if (auto* dev = systems_.MutableNpcDevelopment().GetDevelopment(npc.id)) {
                npc.development_stage = dev->current_stage;
                npc.current_activity = dev->current_job;
                npc.current_location = dev->current_house_id;
                callbacks_.push_hint(
                    "【成长】" + npc.id + " 进入阶段 " + std::to_string(dev->current_stage) + "。",
                    2.4f);
                const bool key_stage = dev->current_stage >= 3;
                const bool first_notify_for_stage = dev->last_notified_stage < dev->current_stage;
                if (key_stage && first_notify_for_stage) {
                    callbacks_.push_hint(
                        "【到场演出】" + npc.display_name + " 的新生活已开启，前往 "
                            + npc.current_location + " 可触发短剧情。",
                        3.0f);
                }
                if (callbacks_.push_notification) {
                    callbacks_.push_notification(
                        "【人物来信】" + npc.display_name
                        + "：我最近有了新变化，来 " + npc.current_location + " 看看吧。");
                }
                if (npc.id == "yunseng") {
                    callbacks_.push_hint(
                        "【云生成长线】当前进度：阶段 "
                            + std::to_string(std::max(0, dev->current_stage)) + "/4。",
                        2.6f);
                }
                if (!previous_location.empty() && previous_location != npc.current_location) {
                    callbacks_.push_hint(
                        "【行踪变化】" + npc.display_name + " 已从 " + previous_location
                            + " 迁往 " + npc.current_location + "。",
                        2.8f);
                }
                dev->last_notified_stage = dev->current_stage;
            }
        } else {
            const int preparing_days = systems_.GetNpcDevelopment().GetPreparingDaysRemaining(
                npc.id, world_state_.MutableClock().Day());
            if (preparing_days > 0 && preparing_days <= 2) {
                callbacks_.push_hint(
                    "【成长准备中】" + npc.display_name + " 正在为下一阶段做准备（约 "
                        + std::to_string(preparing_days) + " 天）。",
                    2.2f);
            }
        }
    }
    if (world_state_.MutableClock().DayInSeason() == 1) {
        const auto rel_rows = LoadNpcRelationshipEvents_();
        for (const auto& row : rel_rows) {
            if (!ContainsTokenCaseInsensitive_(row.type, "friend")
                && !ContainsTokenCaseInsensitive_(row.type, "mentor")) {
                continue;
            }
            if (!SeasonMatches_(row.trigger_season, world_state_.MutableClock().Season())) {
                continue;
            }
            const std::string event_key = row.id + "@"
                + std::to_string(CurrentSeasonKey_(world_state_.MutableClock()));
            if (state_.npc_relationship_event_triggered_keys.contains(event_key)) {
                continue;
            }

            CloudSeamanor::engine::NpcActor* npc_a = nullptr;
            CloudSeamanor::engine::NpcActor* npc_b = nullptr;
            for (auto& npc : world_state_.MutableNpcs()) {
                if (npc.id == row.npc_a) npc_a = &npc;
                if (npc.id == row.npc_b) npc_b = &npc;
            }
            if (npc_a == nullptr || npc_b == nullptr) {
                continue;
            }

            npc_a->current_activity = row.shared_schedule_tag.empty() ? npc_a->current_activity : row.shared_schedule_tag;
            npc_b->current_activity = row.shared_schedule_tag.empty() ? npc_b->current_activity : row.shared_schedule_tag;
            if (auto* dev_a = systems_.MutableNpcDevelopment().GetDevelopment(npc_a->id)) {
                dev_a->npc_relationships[npc_b->id] += row.result_relationship_value;
            }
            if (auto* dev_b = systems_.MutableNpcDevelopment().GetDevelopment(npc_b->id)) {
                dev_b->npc_relationships[npc_a->id] += row.result_relationship_value;
            }

            MailOrderEntry entry;
            entry.item_id = "";
            entry.count = 1;
            entry.deliver_day = world_state_.MutableClock().Day();
            entry.claimed = false;
            entry.opened = false;
            entry.receipt_sent = false;
            entry.source_rule_id = row.mail_id.empty() ? row.id : row.mail_id;
            entry.sender = "庄园布告";
            entry.subject = "居民关系新动态";
            entry.body = npc_a->display_name + " 与 " + npc_b->display_name
                + " 的关系出现了新变化，最近常在 " + npc_a->current_location + " 一带共同活动。";
            world_state_.MutableMailOrders().push_back(std::move(entry));

            callbacks_.push_hint(
                "【关系动态】" + npc_a->display_name + " 与 " + npc_b->display_name + " 的关系更近了一步。",
                2.8f);
            state_.npc_relationship_event_triggered_keys.insert(event_key);
            break;  // quarterly "small amount": only one event per season start
        }
    }
    SyncNpcHouseVariantInteractables_();
    const auto* today_festival = systems_.GetFestivals().GetTodayFestival();
    world_state_.SetActiveFestivalId(today_festival ? today_festival->id : "");
    world_state_.SetFestivalNoticeText(systems_.GetFestivals().GetNoticeText());
    callbacks_.push_hint(
        "【灵气晨报】今日灵气阶段："
            + std::string(systems_.GetCloud().SpiritEnergy() >= 180 ? "丰盈"
                : systems_.GetCloud().SpiritEnergy() >= 120 ? "活跃"
                : systems_.GetCloud().SpiritEnergy() >= 60 ? "平稳" : "低潮")
            + "；潮汐预报："
            + std::string(systems_.GetCloud().TideCountdownDays(world_state_.MutableClock().Day()) <= 0 ? "大潮日"
                : systems_.GetCloud().TideCountdownDays(world_state_.MutableClock().Day()) <= 2 ? "小潮将近" : "平潮期")
            + "。",
        3.0f);
    if (today_festival != nullptr) {
        auto& unlocks = world_state_.MutableRecipeUnlocks();
        // P0-002: 第二解锁来源（节日）——节日当天赠送一份社交向料理配方。
        if (!unlocks["cook_fish_stew"]) {
            unlocks["cook_fish_stew"] = true;
            systems_.GetWorkshop().UnlockRecipe("cook_fish_stew");
            callbacks_.push_hint("节日灵感：已解锁食谱【鱼汤】。", 2.6f);
        }
        if (state_.festival_quick_flow_hint_day != world_state_.MutableClock().Day()) {
            callbacks_.push_hint(
                "【节日安排】今日核心流程：" + today_festival->activity
                    + "，快速参与即可领取 " + today_festival->reward + "。",
                3.2f);
            state_.festival_quick_flow_hint_day = world_state_.MutableClock().Day();
        }
    }
    if (today_festival != nullptr) {
        Event festival_event;
        festival_event.type = "FestivalActiveEvent";
        festival_event.data["festival_id"] = today_festival->id;
        festival_event.data["festival_name"] = today_festival->name;
        festival_event.data["season"] = std::to_string(static_cast<int>(world_state_.MutableClock().Season()));
        festival_event.data["day_in_season"] = std::to_string(world_state_.MutableClock().DayInSeason());
        festival_event.data["day"] = std::to_string(world_state_.MutableClock().Day());
        GlobalEventBus().Emit(festival_event);
    }

    for (const auto& rule : state_.mail_trigger_rules) {
        if (!rule.enabled) continue;
        if (!IsMailRuleAllowedByCooldown_(
                rule,
                world_state_.MutableClock(),
                state_.mail_rule_last_trigger_day,
                state_.mail_rule_last_trigger_season_key,
                state_.mail_rule_triggered_once)) {
            continue;
        }
        if (!ShouldTriggerMailRule_(
                rule,
                world_state_.MutableClock(),
                world_state_.GetActiveFestivalId(),
                world_state_.GetNpcs())) {
            continue;
        }
        MailOrderEntry entry;
        entry.item_id = rule.item_id;
        entry.count = std::max(1, rule.count);
        entry.deliver_day = world_state_.MutableClock().Day() + std::max(0, rule.delay_days);
        entry.claimed = false;
        entry.opened = false;
        entry.receipt_sent = false;
        entry.source_rule_id = rule.id;
        entry.sender = rule.sender_template.empty()
            ? "商会"
            : ApplyMailTemplate_(rule.sender_template, rule, world_state_.MutableClock().Day());
        entry.subject = rule.subject_template.empty()
            ? (ItemDisplayName(rule.item_id) + " x" + std::to_string(rule.count))
            : ApplyMailTemplate_(rule.subject_template, rule, world_state_.MutableClock().Day());
        entry.body = rule.body_template.empty()
            ? "附件已备好，请在邮件面板领取。"
            : ApplyMailTemplate_(rule.body_template, rule, world_state_.MutableClock().Day());
        world_state_.MutableMailOrders().push_back(std::move(entry));
        state_.mail_rule_last_trigger_day[rule.id] = world_state_.MutableClock().Day();
        state_.mail_rule_last_trigger_season_key[rule.id] = CurrentSeasonKey_(world_state_.MutableClock());
        state_.mail_rule_triggered_once[rule.id] = true;
        if (!rule.hint_text.empty()) {
            callbacks_.push_hint(rule.hint_text, 2.6f);
        }
    }

    const int current_day = world_state_.MutableClock().Day();
    const bool festival_day_active = !world_state_.GetActiveFestivalId().empty();
    if (!festival_day_active
        && state_.deferred_story_due_to_festival_day > 0
        && state_.deferred_story_due_to_festival_day < current_day
        && !plot_system_.IsPlaying()) {
        plot_system_.CheckPlotTriggers();
        callbacks_.push_hint("【剧情恢复】昨日因节日顺延的关键剧情，今日已恢复触发。", 3.0f);
        state_.deferred_story_due_to_festival_day = -1;
    }
    if (festival_day_active && !plot_system_.IsPlaying()) {
        const auto pending_plots = plot_system_.GetPendingPlots();
        if (!pending_plots.empty()) {
            state_.deferred_story_due_to_festival_day = current_day;
            callbacks_.push_hint(
                "【剧情顺延】今日节日优先，关键剧情将顺延至次日，避免与你的节庆安排冲突。",
                3.2f);
        }
    }
    if (last_reset_day_ != current_day) {
        if (!festival_day_active) {
            ResetPlotsWateredState(world_state_, RefreshTeaPlotVisual);
        } else if (callbacks_.push_hint) {
            callbacks_.push_hint("【节日优待】今日农务压力已冻结，地块浇水状态将顺延一天。", 2.8f);
        }
        ResetDailyInteractionState(world_state_,
                                   systems_.GetCloud().CurrentState() == CloudSeamanor::domain::CloudState::Clear
                                       ? 0 : 1);
        last_reset_day_ = current_day;
    }
    if (!festival_day_active) {
        crop_growth_.HandleDailyDiseaseAndPest(world_state_);
    } else if (callbacks_.push_hint) {
        callbacks_.push_hint("【节日优待】今日不结算额外病虫害与喂养压力。", 2.4f);
    }

    for (auto& plot : world_state_.MutableTeaPlots()) {
        if (plot.sprinkler_installed) {
            if (plot.sprinkler_days_left > 0) {
                --plot.sprinkler_days_left;
            }
            if (plot.sprinkler_days_left <= 0) {
                plot.sprinkler_installed = false;
            } else if (plot.seeded && !plot.ready) {
                plot.watered = true;
                RefreshTeaPlotVisual(plot, false);
            }
        }
    }

    if (season_changed_event.changed) {
        crop_growth_.HandleSeasonChanged(world_state_, current_season);
        callbacks_.push_hint(
            "季节更替：" + CloudSeamanor::domain::GameClock::SeasonName(season_changed_event.from)
            + " -> " + CloudSeamanor::domain::GameClock::SeasonName(season_changed_event.to),
            2.4f);
    }
    // B4-033: 独立灵界管理器负责稀有节点刷新。
    spirit_realm_manager_.RefreshDailyNodes(world_state_, systems_.GetCloud());
    // A-11 洒水器：对附近地块提供晨间自动浇水
    ApplySprinklerAutoWater(world_state_.MutableTeaPlots(), sprinkler_radius);
    for (auto& p : world_state_.MutableTeaPlots()) {
        if (p.seeded && p.watered && !p.ready) {
            RefreshTeaPlotVisual(p, false);
        }
    }

    // A-30/B-2 委托面板：每日 6:00 后自动接取委托类任务。
    quest_manager_.RefreshByTimeslot(world_state_.MutableRuntimeQuests(), world_state_.MutableClock().Hour());

    // C-9/C-10：交由独立系统负责每日结算。
    coop_system_.DailyUpdate(world_state_);
    barn_system_.DailyUpdate(world_state_);
    if (world_state_.GetLivestockEggsToday() > 0 || world_state_.GetLivestockMilkToday() > 0) {
        callbacks_.push_hint(
            "畜棚日结：鸡蛋 x" + std::to_string(world_state_.GetLivestockEggsToday())
                + "，牛奶 x" + std::to_string(world_state_.GetLivestockMilkToday()) + "。",
            2.4f);
    } else {
        callbacks_.push_hint("畜棚今日未投喂，未产出鸡蛋/牛奶。", 2.0f);
    }
    inn_system_.DailySettlement(world_state_, world_state_.MutableMainHouseRepair().level);
    callbacks_.push_hint(
        "客栈日结：来访 " + std::to_string(world_state_.GetInnVisitorsToday())
            + " 人，收益 +" + std::to_string(world_state_.GetInnIncomeToday())
            + "，口碑 " + std::to_string(world_state_.GetInnReputation()) + "。",
        2.3f);
    {
        // J18: Decoration score drives 3 business lines; comfort uses daily-refresh buff + small flat recovery.
        const int decor = world_state_.GetDecorationScore();
        const float comfort_flat = std::clamp(static_cast<float>(decor) * 0.08f, 0.0f, 8.0f);
        if (comfort_flat > 0.0f) {
            RecoverStaminaScaled_(comfort_flat);
        }
        CloudSeamanor::domain::RuntimeBuff comfort;
        comfort.id = "home_comfort_daily";
        comfort.remaining_seconds = 60.0f * 60.0f * 24.0f;
        comfort.stamina_recovery_multiplier =
            1.0f + std::clamp(static_cast<float>(decor) * 0.002f, 0.0f, 0.12f);
        comfort.stamina_cost_multiplier =
            1.0f - std::clamp(static_cast<float>(decor) * 0.0015f, 0.0f, 0.10f);
        world_state_.MutableBuffs().ApplyBuff(comfort);
        callbacks_.push_hint(
            "家居舒适度（装饰 " + std::to_string(decor) + "）：体力恢复 +" + std::to_string(static_cast<int>(comfort_flat))
                + "，恢复倍率 x" + std::to_string(comfort.stamina_recovery_multiplier).substr(0, 4)
                + "，消耗倍率 x" + std::to_string(comfort.stamina_cost_multiplier).substr(0, 4),
            2.0f);
        const int day = world_state_.MutableClock().Day();
        auto& fr = world_state_.MutableFestivalRuntime();
        if (fr.mid_autumn_regen_bonus_until_day >= day) {
            RecoverStaminaScaled_(12.0f);
            callbacks_.push_hint("【中秋·月相】额外体力滋养 +12。", 1.7f);
        }
        if (fr.double_ninth_chrysanthemum_day == day) {
            RecoverStaminaScaled_(8.0f);
            callbacks_.push_hint("【重阳·菊花酒】体力温润 +8。", 1.7f);
        }
    }

    // 日切汇总通知（可追溯）：便于玩家理解因果，也便于调试定位“哪里漏结算了”。
    if (callbacks_.push_notification) {
        // R7-002 契约：字段顺序固定为
        // 1) 云海 2) 客栈 3) 畜棚 4) 工坊 5) 舒适度 6) 病虫害
        const std::string prefix = config_.GetString("daily_summary_prefix", "日切汇总：");
        const std::string sep = config_.GetString("daily_summary_separator", " | ");
        const std::string inn_income_unit = config_.GetString("daily_summary_inn_income_unit", "+");
        const std::string visitor_prefix = config_.GetString("daily_summary_inn_visitor_prefix", "来访 ");
        const std::string visitor_suffix = config_.GetString("daily_summary_inn_visitor_suffix", "人");
        const std::string egg_label = config_.GetString("daily_summary_egg_label", "蛋x");
        const std::string milk_label = config_.GetString("daily_summary_milk_label", "奶x");
        const std::string workshop_label = config_.GetString("daily_summary_workshop_label", "工坊待领取 茶包x");
        const std::string decor_label = config_.GetString("daily_summary_decor_label", "装饰");
        const std::string comfort_rec_prefix = config_.GetString("daily_summary_comfort_recovery_prefix", "恢复x");
        const std::string comfort_cost_prefix = config_.GetString("daily_summary_comfort_cost_prefix", "消耗x");
        const std::string disease_pest_sep = config_.GetString("daily_summary_disease_pest_separator", "/");
        const std::string cloud_label = config_.GetString("daily_summary_cloud_label", "云海 ");
        const std::string inn_label = config_.GetString("daily_summary_inn_label", "客栈 ");
        const std::string livestock_label = config_.GetString("daily_summary_livestock_label", "畜棚 ");
        const std::string purify_label = config_.GetString("daily_summary_purify_label", "净化回流 ");
        const std::string comfort_label = config_.GetString("daily_summary_comfort_label", "舒适度 ");
        const std::string disease_pest_label = config_.GetString("daily_summary_disease_pest_label", "病虫 ");

        int disease = 0;
        int pest = 0;
        for (const auto& p : world_state_.MutableTeaPlots()) {
            if (p.disease) ++disease;
            if (p.pest) ++pest;
        }
        const int decor = world_state_.GetDecorationScore();
        const int inn_income = world_state_.GetInnIncomeToday();
        const int eggs = world_state_.GetLivestockEggsToday();
        const int milk = world_state_.GetLivestockMilkToday();
        const int workshop_ready = std::max(0, world_state_.MutableTeaMachine().queued_output);
        const int purify_days = std::max(0, world_state_.GetPurifyReturnDays());
        const int purify_spirits = std::max(0, world_state_.GetPurifyReturnSpirits());
        const float comfort_rec = 1.0f + std::clamp(static_cast<float>(decor) * 0.002f, 0.0f, 0.12f);
        const float comfort_cost = 1.0f - std::clamp(static_cast<float>(decor) * 0.0015f, 0.0f, 0.10f);
        const std::string comfort_rec_text = std::to_string(comfort_rec).substr(0, 4);
        const std::string comfort_cost_text = std::to_string(comfort_cost).substr(0, 4);
        callbacks_.push_notification(
            prefix
            + cloud_label + systems_.GetCloud().CurrentStateText()
            + sep + inn_label + inn_income_unit + std::to_string(inn_income)
            + "（" + visitor_prefix + std::to_string(world_state_.GetInnVisitorsToday()) + visitor_suffix + "）"
            + sep + livestock_label + egg_label + std::to_string(eggs) + " " + milk_label + std::to_string(milk)
            + sep + workshop_label + std::to_string(workshop_ready)
            + sep + purify_label + std::to_string(purify_days) + "天/" + std::to_string(purify_spirits) + "灵"
            + sep + comfort_label + decor_label + std::to_string(decor)
            + "（" + comfort_rec_prefix + comfort_rec_text + " " + comfort_cost_prefix + comfort_cost_text + "）"
            + sep + disease_pest_label + std::to_string(disease) + disease_pest_sep + std::to_string(pest));
    }

    // R7-003: retention_day_cycle_completed
    EmitRetentionEvent_(
        "retention_day_cycle_completed",
        "day=" + std::to_string(world_state_.MutableClock().Day()));

    // Accumulate weekly metrics for R28-001 weekly summary.
    state_.retention_weekly_inn_income += world_state_.GetInnIncomeToday();
    state_.retention_weekly_eggs += world_state_.GetLivestockEggsToday();
    state_.retention_weekly_milk += world_state_.GetLivestockMilkToday();
    state_.retention_weekly_workshop_ready_sum += std::max(0, world_state_.MutableTeaMachine().queued_output);

    // R7-003: retention_cross_system_gain (deco-linked business gain visible today)
    if (world_state_.GetDecorationScore() > 0
        && (world_state_.GetInnIncomeToday() > 0
            || world_state_.GetLivestockEggsToday() > 0
            || world_state_.GetLivestockMilkToday() > 0)) {
        EmitRetentionEvent_(
            "retention_cross_system_gain",
            "day=" + std::to_string(world_state_.MutableClock().Day())
                + ",decor=" + std::to_string(world_state_.GetDecorationScore())
                + ",inn=" + std::to_string(world_state_.GetInnIncomeToday())
                + ",eggs=" + std::to_string(world_state_.GetLivestockEggsToday())
                + ",milk=" + std::to_string(world_state_.GetLivestockMilkToday()));
    }

    // R28-002: midterm goal commitment (first time reaching one of 3 milestone lines).
    if (!state_.retention_midterm_goal_committed) {
        const bool house_committed = world_state_.MutableMainHouseRepair().level >= 2;
        const auto stage = world_state_.MutableSocial().relationship.stage;
        const bool social_committed =
            stage == CloudSeamanor::domain::RelationshipStage::Engaged
            || stage == CloudSeamanor::domain::RelationshipStage::WeddingScheduled
            || stage == CloudSeamanor::domain::RelationshipStage::Married;
        const bool spirit_realm_committed =
            world_state_.GetSpiritRealmDailyRemaining() < world_state_.GetSpiritRealmDailyMax();
        if (house_committed || social_committed || spirit_realm_committed) {
            state_.retention_midterm_goal_committed = true;
            std::string line = "house";
            if (social_committed) line = "social";
            else if (spirit_realm_committed) line = "spirit_realm";
            if (callbacks_.push_hint) {
                const std::string hint_prefix = config_.GetString("midterm_goal_commit_hint_prefix", "中期目标已形成承诺（");
                const std::string hint_suffix = config_.GetString("midterm_goal_commit_hint_suffix", "线）。");
                const std::string line_house = config_.GetString("midterm_goal_line_house", "主屋升级");
                const std::string line_social = config_.GetString("midterm_goal_line_social", "关系推进");
                const std::string line_spirit = config_.GetString("midterm_goal_line_spirit_realm", "灵界推进");
                const std::string line_text =
                    (line == "social") ? line_social : ((line == "spirit_realm") ? line_spirit : line_house);
                callbacks_.push_hint(hint_prefix + line_text + hint_suffix, 2.2f);
            }
            EmitRetentionEvent_(
                "retention_midterm_goal_committed",
                "day=" + std::to_string(world_state_.MutableClock().Day()) + ",line=" + line);
        }
    }

    // R28-002: daily 1 main goal + 2 side goals recommendation contract.
    if (callbacks_.push_notification) {
        const std::string suggest_prefix = config_.GetString("midterm_goal_suggestion_prefix", "中期目标建议：");
        const std::string main_prefix = config_.GetString("midterm_goal_main_prefix", "主目标[");
        const std::string main_suffix = config_.GetString("midterm_goal_main_suffix", "]");
        const std::string side_prefix = config_.GetString("midterm_goal_side_prefix", "  副目标[");
        const std::string side_sep = config_.GetString("midterm_goal_side_separator", " / ");
        const std::string side_suffix = config_.GetString("midterm_goal_side_suffix", "]");

        const std::array<std::string, 3> kGoalLines = {
            config_.GetString("midterm_goal_line_house", "主屋升级"),
            config_.GetString("midterm_goal_line_social", "关系推进"),
            config_.GetString("midterm_goal_line_spirit_realm", "灵界推进"),
        };
        const int rotate = std::max(0, (world_state_.MutableClock().Day() - 1) % 3);
        const int side_a = (rotate + 1) % 3;
        const int side_b = (rotate + 2) % 3;
        callbacks_.push_notification(
            suggest_prefix
            + main_prefix + kGoalLines[static_cast<std::size_t>(rotate)] + main_suffix
            + side_prefix + kGoalLines[static_cast<std::size_t>(side_a)] + side_sep
            + kGoalLines[static_cast<std::size_t>(side_b)] + side_suffix);
    }

    // R28-001 + R7-003: weekly summary generation at end of each 7-day cycle.
    if (world_state_.MutableClock().Day() % 7 == 0) {
        const int inn = state_.retention_weekly_inn_income;
        const int crop = state_.retention_weekly_eggs + state_.retention_weekly_milk;
        const int process = state_.retention_weekly_workshop_ready_sum;
        const int battle = state_.retention_weekly_battle_victories;
        const int total = std::max(1, inn + crop + process + battle);
        const int inn_ratio = static_cast<int>(std::round(static_cast<float>(inn) * 100.0f / static_cast<float>(total)));
        const int crop_ratio = static_cast<int>(std::round(static_cast<float>(crop) * 100.0f / static_cast<float>(total)));
        const int process_ratio = static_cast<int>(std::round(static_cast<float>(process) * 100.0f / static_cast<float>(total)));
        const int battle_ratio = std::max(0, 100 - inn_ratio - crop_ratio - process_ratio);
        int max_heart_level = 0;
        for (const auto& npc : world_state_.MutableNpcs()) {
            max_heart_level = std::max(max_heart_level, npc.heart_level);
        }
        const int growth_lines =
            (inn > 0 ? 1 : 0) + (crop > 0 ? 1 : 0) + (process > 0 ? 1 : 0) + (battle > 0 ? 1 : 0);
        const bool dual_engine_growth = growth_lines >= 2;
        const std::string week_prefix = config_.GetString("weekly_report_week_prefix", "W");
        const std::string week_label = week_prefix + std::to_string(std::max(1, world_state_.MutableClock().Day() / 7));

        const std::string income_ratio_prefix = config_.GetString("weekly_report_income_ratio_prefix", " 收入占比 ");
        const std::string income_sep = config_.GetString("weekly_report_income_ratio_source_separator", "/");
        const std::string src_inn = config_.GetString("weekly_report_source_inn_label", "客栈");
        const std::string src_crop = config_.GetString("weekly_report_source_crop_label", "作物");
        const std::string src_process = config_.GetString("weekly_report_source_process_label", "加工");
        const std::string src_battle = config_.GetString("weekly_report_source_battle_label", "战斗");
        const std::string percent_suffix = config_.GetString("weekly_report_percent_suffix", "%");
        const std::string section_sep = config_.GetString("weekly_report_section_separator", "；");
        const std::string rel_prefix = config_.GetString("weekly_report_relationship_prefix", "关系最高心级 ");
        const std::string dual_prefix = config_.GetString("weekly_report_dual_engine_prefix", "双引擎增长 ");
        const std::string yes_text = config_.GetString("weekly_report_yes_text", "是");
        const std::string no_text = config_.GetString("weekly_report_no_text", "否");
        const std::string weekly_report =
            week_label
            + income_ratio_prefix
            + src_inn + std::to_string(inn_ratio) + percent_suffix
            + income_sep + src_crop + std::to_string(crop_ratio) + percent_suffix
            + income_sep + src_process + std::to_string(process_ratio) + percent_suffix
            + income_sep + src_battle + std::to_string(battle_ratio) + percent_suffix
            + section_sep + rel_prefix + std::to_string(max_heart_level)
            + section_sep + dual_prefix + (dual_engine_growth ? yes_text : no_text);

        if (callbacks_.push_notification) {
            const std::string weekly_prefix = config_.GetString("weekly_report_notification_prefix", "本周经营报告：");
            callbacks_.push_notification(weekly_prefix + weekly_report);
        }
        auto& reports = world_state_.MutableWeeklyReports();
        reports.push_back(weekly_report);
        if (reports.size() > 4) {
            reports.erase(reports.begin(), reports.begin() + static_cast<long long>(reports.size() - 4));
        }
        EmitRetentionEvent_(
            "retention_weekly_summary_generated",
            "day=" + std::to_string(world_state_.MutableClock().Day())
                + ",inn=" + std::to_string(state_.retention_weekly_inn_income)
                + ",eggs=" + std::to_string(state_.retention_weekly_eggs)
                + ",milk=" + std::to_string(state_.retention_weekly_milk)
                + ",workshop=" + std::to_string(state_.retention_weekly_workshop_ready_sum)
                + ",battle=" + std::to_string(state_.retention_weekly_battle_victories));
        state_.retention_weekly_inn_income = 0;
        state_.retention_weekly_eggs = 0;
        state_.retention_weekly_milk = 0;
        state_.retention_weekly_workshop_ready_sum = 0;
        state_.retention_weekly_battle_victories = 0;
    }

    const auto grant_achievement_reward = [this](const std::string& achievement_id) {
        if (achievement_id == "home_designer") {
            world_state_.MutableGold() += 200;
            callbacks_.push_hint("成就奖励：金币 +200", 1.8f);
        } else if (achievement_id == "small_tycoon") {
            world_state_.MutableInventory().AddItem("TeaPack", 2);
            callbacks_.push_hint("成就奖励：茶包 x2", 1.8f);
        } else if (achievement_id == "beast_bond_max") {
            world_state_.MutableGold() += 300;
            callbacks_.push_hint("成就奖励：金币 +300", 1.8f);
        }
    };
    achievement_system_.EvaluateDaily(
        world_state_.MutableAchievements(),
        world_state_.GetDecorationScore(),
        world_state_.GetGold(),
        world_state_.MutableSpiritBeast(),
        [this](const std::string& text) { callbacks_.push_hint(text, 2.4f); },
        grant_achievement_reward);
    if (!world_state_.GetModHooks().empty()) {
        callbacks_.push_hint("Mod Hook 触发: OnDayChanged x"
            + std::to_string(world_state_.GetModHooks().size()), 1.4f);
    }

    // B-16 价格波动：每周重置统计，按供需调整价格。
    if (world_state_.MutableClock().Day() % 7 == 1) {
        world_state_.MutableWeeklyBuyCount().clear();
        world_state_.MutableWeeklySellCount().clear();
    }
    for (auto& price : world_state_.MutablePriceTable()) {
        const int buy_count = world_state_.MutableWeeklyBuyCount()[price.item_id];
        const int sell_count = world_state_.MutableWeeklySellCount()[price.item_id];
        if (buy_count > 10 && price.buy_price > 0) {
            price.buy_price = std::min(price.buy_price * 11 / 10, price.buy_price * 2);
        }
        if (sell_count > 20 && price.sell_price > 0) {
            price.sell_price = std::max(1, price.sell_price * 95 / 100);
        }
    }
    // B-17 每日杂货店随机进货。
    world_state_.MutableDailyGeneralStoreStock().clear();
    for (const auto& e : world_state_.GetPriceTable()) {
        if (e.buy_from == "general_store") {
            world_state_.MutableDailyGeneralStoreStock().push_back(e.item_id);
            if (world_state_.MutableDailyGeneralStoreStock().size() >= 3) break;
        }
    }
    quest_manager_.EvaluateProgress(
        world_state_.MutableRuntimeQuests(),
        world_state_.MutableInventory(),
        systems_.GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm),
        world_state_.MutableGold(),
        &world_state_.MutableStamina(),
        [this](const std::string& item_id, int count) {
            world_state_.MutableInventory().AddItem(item_id, count);
        },
        [this](int favor_delta) {
            auto& npcs = world_state_.MutableNpcs();
            if (!npcs.empty()) {
                npcs.front().favor += favor_delta;
            }
        },
        [this](const std::string& text) {
            callbacks_.push_hint(text, 2.2f);
        });

    // BE-027 NPC 心情系统：按天气/季节偏好/送礼行为刷新，并影响当日互动倍率与语气。
    const auto weather = systems_.GetCloud().CurrentState();
    const auto season = world_state_.MutableClock().Season();
    auto PreferredSeasonForNpc_ = [](const std::string& npc_id)
        -> std::optional<CloudSeamanor::domain::Season> {
        using CloudSeamanor::domain::Season;
        if (npc_id == "acha") return Season::Spring;
        if (npc_id == "xiaoman") return Season::Summer;
        if (npc_id == "lin") return Season::Autumn;
        if (npc_id == "wanxing") return Season::Winter;
        return std::nullopt;
    };
    for (auto& npc : world_state_.MutableNpcs()) {
        // 基础：天气驱动
        switch (weather) {
        case CloudSeamanor::domain::CloudState::Clear:
        case CloudSeamanor::domain::CloudState::Tide:
            npc.mood = NpcMood::Happy;
            break;
        case CloudSeamanor::domain::CloudState::DenseCloud:
        case CloudSeamanor::domain::CloudState::Mist:
        default:
            npc.mood = NpcMood::Normal;
            break;
        }

        // 季节偏好：喜欢当前季节则更愉快
        if (const auto pref = PreferredSeasonForNpc_(npc.id); pref && *pref == season) {
            npc.mood = NpcMood::Happy;
        }

        // 玩家行为：最近送礼更开心，长时间未送礼更低落
        const int days_since_gift = std::max(0, world_state_.MutableClock().Day() - npc.last_gift_day);
        if (npc.last_gift_day == world_state_.MutableClock().Day()
            || npc.last_gift_day == (world_state_.MutableClock().Day() - 1)) {
            npc.mood = NpcMood::Happy;
        } else if (npc.last_gift_day > 0 && days_since_gift >= 3) {
            npc.mood = NpcMood::Sad;
        }
    }

    // BE-075: 四季 BGM 路由：换日时按季节切换；大潮日覆盖为 tide 主题。
    std::string bgm_path = SeasonalBgmPath_(world_state_.MutableClock().Season());
    if (today_festival != nullptr) {
        bgm_path = "assets/audio/bgm/festival_theme.wav";
    } else if (systems_.GetCloud().CurrentState() == CloudSeamanor::domain::CloudState::Tide) {
        bgm_path = "assets/audio/bgm/tide_theme.wav";
    }
    if (bgm_path != current_bgm_path_) {
        if (callbacks_.play_bgm) {
            callbacks_.play_bgm(bgm_path, true, 0.8f, 0.5f);
        }
        current_bgm_path_ = bgm_path;
    }

    const auto& mail_orders = world_state_.GetMailOrders();
    int arrived_mail_count = 0;
    for (const auto& order : mail_orders) {
        if (order.deliver_day <= world_state_.MutableClock().Day()) {
            ++arrived_mail_count;
        }
    }
    if (arrived_mail_count > 0) {
        callbacks_.push_hint("邮件已送达：" + std::to_string(arrived_mail_count) + " 封，请在邮件面板领取。", 2.8f);
    }

    if (const auto it = world_state_.GetSkillBranches().find("灵农");
        it != world_state_.GetSkillBranches().end() && it->second == "conservation") {
        world_state_.MutableHunger().RestoreFromFood(6, 1.0f);
        callbacks_.push_hint("【分支加成】灵农·节用：次日额外恢复少量饱食。", 2.4f);
    }

    EnsureSkillBranchesUnlocked_(world_state_, systems_.GetSkills(), callbacks_.push_hint);
    TryUnlockDiaryEntries_(world_state_, world_state_.GetActiveFestivalId(), callbacks_.push_hint);

    systems_.CheckContractUnlocks();
    const int contract_count = systems_.GetContracts().CompletedVolumeCount();
    if (contract_count > last_contract_completed_count_) {
        Event ev;
        ev.type = "OnContractCompleted";
        ev.data["completed_count"] = std::to_string(contract_count);
        GlobalEventBus().Emit(ev);
        last_contract_completed_count_ = contract_count;
    }

    TrySpiritBeastWateringAid(
        world_state_.MutableSpiritBeast(),
        world_state_.MutableSpiritBeastWateredToday(),
        world_state_.MutableTeaPlots(),
        world_state_.MutableStamina(),
        RefreshTeaPlotVisual,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        CloudSeamanor::infrastructure::Logger::Info);

    // H4 自动存档：每日切换完成后自动写入当前槽位，失败仅提示不打断游戏。
    if (SaveGameToSlot(active_save_slot_)) {
        callbacks_.push_hint("已完成每日自动存档。", 1.8f);
        CloudSeamanor::infrastructure::Logger::Info(
            "Auto-save succeeded at day change. slot=" + std::to_string(active_save_slot_));
    } else {
        callbacks_.push_hint("自动存档失败，请手动按 F6 保存。", 2.4f);
        CloudSeamanor::infrastructure::Logger::Warning(
            "Auto-save failed at day change. slot=" + std::to_string(active_save_slot_));
    }

}

// ============================================================================
// 【GameRuntime::OnPlayerMoved】玩家移动
// ============================================================================
void GameRuntime::OnPlayerMoved(float delta_seconds, const sf::Vector2f& direction) {
    ApplyQuestSkillExploreEffects_(delta_seconds);
    player_movement_.Update(world_state_, delta_seconds, direction);
}

// ============================================================================
// 【GameRuntime::OnPlayerInteracted】玩家交互回调
// ============================================================================
void GameRuntime::OnPlayerInteracted(const CloudSeamanor::domain::Interactable& target) {
    if (!ConsumeStaminaScaled_(world_state_.GetConfig().stamina_interact_cost)) {
        callbacks_.push_hint("体力不足，先补充后再交互。", 1.8f);
        return;
    }
    if (HasQuestSkill_("qs_wind_step")) {
        // Small exploration burst after any successful interaction.
        state_.quest_wind_step_remaining_seconds = std::max(state_.quest_wind_step_remaining_seconds, 1.2f);
    }

    if (target.Type() == CloudSeamanor::domain::InteractableType::GatheringNode) {
        systems_.GetPickups().SpawnPickup(
            adapter::ToDomain(target.Shape().getPosition()) + CloudSeamanor::domain::Vec2f{14.0f, -10.0f},
            target.RewardItem(),
            target.RewardAmount());
        if (const auto it = world_state_.GetSkillBranches().find("灵觅");
            it != world_state_.GetSkillBranches().end() && it->second == "swiftstep") {
            systems_.GetPickups().SpawnPickup(
                adapter::ToDomain(target.Shape().getPosition()) + CloudSeamanor::domain::Vec2f{22.0f, -6.0f},
                target.RewardItem(),
                1);
        }
        const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();
        const float beast_share = world_state_.MutableSpiritBeast().daily_interacted
            ? GameConstants::SpiritBeast::AssistHarvestMultiplier
            : GameConstants::Runtime::DefaultTimeScale;
        if (systems_.GetSkills().AddExp(
                CloudSeamanor::domain::SkillType::SpiritForage,
                GameConstants::Runtime::SpiritForageExpGain,
                cloud_density,
                GameConstants::Runtime::DefaultTimeScale,
                beast_share)) {
            world_state_.SetLevelUpOverlayActive(true);
            world_state_.SetLevelUpOverlayTimer(GameConstants::Ui::LevelUp::OverlayDuration);
            world_state_.SetLevelUpSkillType(CloudSeamanor::domain::SkillType::SpiritForage);
            callbacks_.push_hint(
                "灵觅技能提升至 Lv." +
                std::to_string(systems_.GetSkills().GetLevel(
                    CloudSeamanor::domain::SkillType::SpiritForage)) + "！",
                GameConstants::Ui::HintDuration::SkillLevelUp);
        }
    }
}

// ============================================================================
// 【GameRuntime::Update】每帧更新
// ============================================================================
void GameRuntime::Update(float delta_seconds) {
    if (in_battle_mode_) {
        const auto player_pos = world_state_.GetPlayer().GetPosition();
        battle_manager_.Update(delta_seconds, player_pos.x, player_pos.y);
        if (!battle_manager_.IsInBattle()) {
            const auto& result = battle_manager_.GetLastResult();
            const bool exit_by_retreat = state_.battle_exit_by_retreat;
            if (state_.tide_festival_battle_active) {
                if (result.victory) {
                    world_state_.MutableInventory().AddItem("legendary_tide", 1);
                    world_state_.MutableInventory().AddItem("tide_heart", 1);
                    world_state_.MutableGold() += 600;
                    Event ev;
                    ev.type = "FestivalTideBossVictoryEvent";
                    ev.data["festival_id"] = "cloud_tide_ritual";
                    ev.data["day"] = std::to_string(world_state_.MutableClock().Day());
                    ev.data["reward_gold"] = "600";
                    ev.data["boss_id"] = "spirit_tide_lord_boss";
                    GlobalEventBus().Emit(ev);
                    callbacks_.push_hint("【大潮祭胜利】获得 传说潮汐印记、潮心与金币 +600。", 3.2f);
                    state_.tide_festival_battle_pending = false;
                } else {
                    callbacks_.push_hint("【大潮祭】本次挑战未成功，今天仍可再次挑战潮灵。", 2.8f);
                }
                state_.tide_festival_battle_active = false;
            }
            // 设计语义对齐：失败/撤退均无经营惩罚，仅输出结算提示。
            if (callbacks_.push_notification) {
                callbacks_.push_notification(BuildBattleOutcomeSummary_(result, exit_by_retreat));
            }
            if (result.victory && result.spirits_purified > 0) {
                world_state_.MutablePurifyReturnDays() =
                    std::max(world_state_.GetPurifyReturnDays(), 3);
                world_state_.MutablePurifyReturnSpirits() =
                    std::max(world_state_.GetPurifyReturnSpirits(), result.spirits_purified);
                if (const auto it = world_state_.GetSkillBranches().find("灵卫");
                    it != world_state_.GetSkillBranches().end() && it->second == "ward") {
                    world_state_.MutablePurifyReturnDays() =
                        std::max(world_state_.GetPurifyReturnDays(), 4);
                } else if (const auto it = world_state_.GetSkillBranches().find("灵卫");
                           it != world_state_.GetSkillBranches().end() && it->second == "barrier") {
                    world_state_.MutableSpiritRealmDailyMax() += 1;
                    world_state_.MutableSpiritRealmDailyRemaining() =
                        std::max(world_state_.GetSpiritRealmDailyRemaining(), world_state_.GetSpiritRealmDailyMax());
                }
                if (callbacks_.push_notification) {
                    callbacks_.push_notification(
                        "【净化回流】灵气结晶 +" + std::to_string(result.spirits_purified)
                        + "，未来 3 天游园生机增强。");
                }
            }
            if (result.victory) {
                state_.retention_weekly_battle_victories += 1;
            }
            if (callbacks_.push_hint) {
                if (exit_by_retreat) {
                    callbacks_.push_hint("你已主动撤退，本次战斗按撤退结算。", 2.2f);
                } else if (result.victory) {
                    callbacks_.push_hint("战斗已完成，奖励已发放并记录到结算通知。", 2.2f);
                } else {
                    callbacks_.push_hint("战斗已结束，本次净化未完成。", 2.2f);
                }
            }
            if (battle_manager_.IsPaused()) {
                battle_manager_.Resume();
            }
            state_.battle_exit_by_retreat = false;
            in_battle_mode_ = false;
            state_.battle_trigger_cooldown_seconds = std::max(state_.battle_trigger_cooldown_seconds, 1.2f);
        }
        return;
    }

    CSM_ZONE_SCOPED;
    const int previous_day = world_state_.MutableClock().Day();
    world_state_.MutableClock().Tick(delta_seconds);
    world_state_.MutableHunger().Tick(delta_seconds);
    world_state_.MutableBuffs().Tick(delta_seconds);
    if (state_.fishing_qte_active) {
        state_.fishing_qte_progress += state_.fishing_qte_velocity * delta_seconds;
        if (state_.fishing_qte_progress >= 1.0f) {
            state_.fishing_qte_progress = 1.0f;
            state_.fishing_qte_velocity *= -1.0f;
        } else if (state_.fishing_qte_progress <= 0.0f) {
            state_.fishing_qte_progress = 0.0f;
            state_.fishing_qte_velocity *= -1.0f;
        }
    }
    systems_.GetCloud().UpdateForecastVisibility(
        world_state_.MutableClock().Day(), world_state_.MutableClock().Hour());

    if (world_state_.MutableClock().Day() != previous_day) {
        OnDayChanged();
    }

    // BE-028 NPC 委托：每日 6:00 刷新 + 完成判定（奖励由对话领取）。
    npc_delivery_.Update(world_state_, world_state_.MutableClock().Day(), world_state_.MutableClock().Hour());

    // BE-029 场景过场动画更新（淡入淡出）
    scene_transition_.Update(delta_seconds);

    // UI-020 云海日报：每日 6:00 自动推送一次（使用通知横幅，点开详面板由前端处理）。
    {
        auto& tutorial = world_state_.MutableTutorial();
        const int day = world_state_.MutableClock().Day();
        const int hour = world_state_.MutableClock().Hour();
        if (hour >= 6 && tutorial.daily_cloud_report_day_shown != day) {
            tutorial.daily_cloud_report_day_shown = day;
            if (callbacks_.push_notification) {
                const auto& cloud = systems_.GetCloud();
                const std::string msg =
                    std::string("☁ 今日云海: ") + cloud.CurrentStateText()
                    + "  作物加成: x" + std::to_string(CloudGrowthMultiplier(cloud.CurrentState()))
                    + "  明日预报: " + (cloud.IsForecastVisible() ? cloud.ForecastStateText() : "22:00后公布");
                callbacks_.push_notification(msg);
            }
        }
    }
    // B-14 灵界自动返回
    if (world_state_.GetInSpiritRealm() && world_state_.MutableClock().Hour() >= 22) {
        world_state_.SetInSpiritRealm(false);
        callbacks_.push_hint("夜深了，你被传送门自动送回主世界。", 2.8f);
    }

    // BATTLE-LOOP: 灵界探索自动接近触发（替代手动按 J），带冷却防止连战刷屏。
    if (state_.battle_trigger_cooldown_seconds > 0.0f) {
        state_.battle_trigger_cooldown_seconds =
            std::max(0.0f, state_.battle_trigger_cooldown_seconds - delta_seconds);
    }
    if (world_state_.GetInSpiritRealm()
        && state_.battle_trigger_cooldown_seconds <= 0.0f
        && !scene_transition_.IsActive()) {
        if (TryEnterBattleByPlayerPosition()) {
            state_.battle_trigger_cooldown_seconds = std::max(1.0f, config_.GetFloat("battle_trigger_cooldown_seconds", 2.0f));
        }
    }

    world_state_.SetSessionTime(world_state_.GetSessionTime() + delta_seconds);

    // 提示计时
    if (world_state_.MutableInteraction().hint_timer > 0.0f) {
        world_state_.MutableInteraction().hint_timer =
            std::max(0.0f, world_state_.MutableInteraction().hint_timer - delta_seconds);
    }

    // 升级动画
    if (world_state_.GetLevelUpOverlayActive()) {
        world_state_.SetLevelUpOverlayTimer(
            world_state_.GetLevelUpOverlayTimer() - delta_seconds);
        if (world_state_.GetLevelUpOverlayTimer() <= 0.0f) {
            world_state_.SetLevelUpOverlayActive(false);
        }
    }

    if (modules_.tutorial) {
        modules_.tutorial->Update(delta_seconds);
    } else {
        CheckTutorialHints();
    }

    // 主线剧情系统更新
    if (plot_system_.IsPlaying()) {
        plot_system_.Update(delta_seconds);
    }

    if (last_cloud_state_ != systems_.GetCloud().CurrentState()) {
        last_cloud_state_ = systems_.GetCloud().CurrentState();
        callbacks_.push_hint(
            "云海气场发生变化：" + systems_.GetCloud().CurrentStateText() + "。" +
            BuildWeatherAdviceText(systems_.GetCloud().CurrentState(),
                                   systems_.GetCloud().IsForecastVisible()),
            GameConstants::Ui::HintDuration::CloudStateChanged);
    }

    // 体力警告
    if (!world_state_.GetLowStaminaWarningActive() && world_state_.MutableStamina().Ratio() <= GameConstants::Player::LowStaminaWarningRatio) {
        world_state_.SetLowStaminaWarningActive(true);
        callbacks_.push_hint("体力偏低，先休息一下或放慢节奏。", GameConstants::Ui::HintDuration::LowStaminaWarning);
    } else if (world_state_.GetLowStaminaWarningActive() && world_state_.MutableStamina().Ratio() > GameConstants::Player::LowStaminaRecoverRatio) {
        world_state_.SetLowStaminaWarningActive(false);
        callbacks_.push_hint("体力已经恢复一些了。", GameConstants::Ui::HintDuration::LowStaminaRecovered);
    }

    UpdateCropGrowth(delta_seconds);
    UpdateWorkshop(delta_seconds);
    if (modules_.pickup) {
        modules_.pickup->Update(delta_seconds);
        modules_.pickup->CollectNearby();
    }
    UpdateNpcs(delta_seconds);
    UpdateSpiritBeast(delta_seconds);
    pet_system_.Update(world_state_, delta_seconds);
    UpdateParticles(delta_seconds);
    UpdateHighlightedInteractable();
    UpdateUi(delta_seconds);
    world_state_.SyncSceneVisuals();
}

void GameRuntime::RenderBattle(sf::RenderWindow& window) {
    if (!in_battle_mode_) {
        return;
    }

    // 使用 BattleRenderer 渲染战斗场景
    battle_manager_.GetRenderer().Render(window, battle_manager_.GetField());

    // BattleUI 仍然负责渲染 HUD（能量条、技能按钮、日志等）
    battle_manager_.GetUI().Draw(window);
}

bool GameRuntime::HandleBattleKey(int skill_slot) {
    if (!in_battle_mode_) {
        return false;
    }
    return battle_manager_.OnSkillKeyPressed(skill_slot, 0.0f, 0.0f);
}

void GameRuntime::ToggleBattlePause() {
    if (!in_battle_mode_) {
        return;
    }
    if (battle_manager_.IsPaused()) {
        battle_manager_.Resume();
    } else {
        battle_manager_.Pause();
    }
}

void GameRuntime::RetreatBattle() {
    if (!in_battle_mode_) {
        return;
    }
    state_.battle_exit_by_retreat = true;
    battle_manager_.Retreat();
}

bool GameRuntime::TryEnterBattleByPlayerPosition() {
    if (in_battle_mode_ || !world_state_.GetInSpiritRealm()) {
        return false;
    }
    const float stamina_cost = std::max(0.0f, config_.GetFloat("battle_enter_stamina_cost", 6.0f));
    const float cost_mul = world_state_.GetHunger().StaminaCostMultiplier()
        * world_state_.GetBuffs().StaminaCostMultiplier();
    const float scaled_stamina_cost = stamina_cost * cost_mul;
    if (scaled_stamina_cost > 0.0f && world_state_.MutableStamina().Current() < scaled_stamina_cost) {
        callbacks_.push_hint("体力不足，无法进入战斗。", 2.2f);
        return false;
    }
    const auto player_pos = world_state_.GetPlayer().GetPosition();
    const int current_day = world_state_.MutableClock().Day();
    const bool tide_boss_ready = state_.tide_festival_battle_pending
        && (state_.tide_festival_battle_day == current_day);

    // 根据当前地图路径确定灵界层数 → 对应 zone_id
    const std::string current_map = tmx_map_.LastLoadedPath();
    std::string zone_id = "zone_spirit_realm_1";  // 默认浅层
    std::string zone_name = "灵界浅层";
    if (current_map.find("spirit_realm_layer3") != std::string::npos) {
        zone_id = "zone_spirit_realm_3";
        zone_name = "灵界核心";
    } else if (current_map.find("spirit_realm_layer2") != std::string::npos) {
        zone_id = "zone_spirit_realm_2";
        zone_name = "灵界深层";
    }

    // 各层随机敌人池（从 zone_table.csv 的 available_spirits 截取）
    static const std::vector<std::string> kLayer1Enemies{
        "spirit_deadbamboo_14", "spirit_leafstorm_18", "spirit_withered_orchid_22"
    };
    static const std::vector<std::string> kLayer2Enemies{
        "spirit_angrystone_21", "spirit_cloudjelly_23", "spirit_misty_ghost_24"
    };
    static const std::vector<std::string> kLayer3Enemies{
        "spirit_vengeful_elite_23", "spirit_jellyfish_24", "spirit_vineking_25", "spirit_shadow_26"
    };
    const std::vector<std::string>& enemy_pool =
        (zone_id == "zone_spirit_realm_3") ? kLayer3Enemies :
        (zone_id == "zone_spirit_realm_2") ? kLayer2Enemies : kLayer1Enemies;

    for (const auto& object : world_state_.MutableInteractables()) {
        if (!IsGenericBattleAnchor_(object)) {
            continue;
        }
        const auto pos = object.Shape().getPosition();
        if (battle_manager_.ShouldTriggerBattle("spirit_beast", pos.x, pos.y, player_pos.x, player_pos.y)) {
            BattleZone zone;
            zone.id = zone_id;
            zone.name = zone_name;
            zone.is_spirit_realm = true;

            // 优先级：BOSS enemy_id（来自TMX） > 大潮祭坛BOSS > 潮汐BOSS > 层敌人池
            std::vector<std::string> enemies;
            std::string battle_hint = "遭遇灵界污染体，进入净化战斗。Q/W/E/R 技能，B/X 撤离。";

            if (tide_boss_ready) {
                zone.id = "zone_boss_tide";
                zone.name = "云海大潮祭坛";
                enemies = {"spirit_cloud_tide_lord"};
                battle_hint = "【大潮祭】潮灵降临，进入祭典决战！";
            } else if (!object.EnemyId().empty()) {
                // TMX 中明确指定了敌人ID（如 BossSpawn），使用该ID
                zone.id = "zone_spirit_realm_3";
                zone.name = "灵界核心";
                enemies = {object.EnemyId()};
                battle_hint = "BOSS现身，进入决战！";
            } else if (object.Label() == "Spirit Beast Zone") {
                // Spirit Beast Zone：层敌人池中随机1-2个
                std::mt19937 rng{static_cast<std::uint32_t>(
                    std::hash<std::string>{}(object.EnemyId() + object.Label()) ^
                    static_cast<std::uint32_t>(current_day * 31))};
                std::vector<std::string> shuffled_pool = enemy_pool;
                std::shuffle(shuffled_pool.begin(), shuffled_pool.end(), rng);
                enemies.push_back(shuffled_pool.front());
                if (shuffled_pool.size() >= 2) {
                    enemies.push_back(shuffled_pool[1]);
                }
            } else {
                // 普通 Spirit Beast：单敌
                std::mt19937 rng{static_cast<std::uint32_t>(
                    std::hash<std::string>{}(object.EnemyId() + std::to_string(current_day)))};
                std::uniform_int_distribution<std::size_t> dist(0, enemy_pool.size() - 1);
                enemies = {enemy_pool[dist(rng)]};
            }

            if (battle_manager_.EnterBattle(zone, enemies)) {
                SyncEquippedWeaponFromSaveAndInventory_();
                (void)ConsumeStaminaScaled_(stamina_cost);
                in_battle_mode_ = true;
                state_.battle_exit_by_retreat = false;
                state_.tide_festival_battle_active = tide_boss_ready;
                callbacks_.push_hint(battle_hint, 2.8f);
                if (stamina_cost > 0.0f) {
                    callbacks_.push_hint(
                        "进入战斗消耗体力 -" + std::to_string(static_cast<int>(scaled_stamina_cost)) + "。",
                        1.8f);
                }
                return true;
            }
        }
    }
    return false;
}

void GameRuntime::CycleSpiritBeastName() {
    static const std::array<const char*, 6> kCandidateNames{
        "灵团", "云丸", "小岚", "团团", "阿雾", "辰团"
    };
    auto& beast = world_state_.MutableSpiritBeast();
    std::size_t idx = 0;
    for (std::size_t i = 0; i < kCandidateNames.size(); ++i) {
        if (beast.custom_name == kCandidateNames[i]) {
            idx = (i + 1) % kCandidateNames.size();
            break;
        }
    }
    beast.custom_name = kCandidateNames[idx];
    callbacks_.push_hint("已为灵兽命名：" + beast.custom_name, 2.2f);
}

void GameRuntime::CycleTrackingContractVolume(int delta) {
    const auto& vols = systems_.GetContracts().Volumes();
    if (vols.empty()) {
        return;
    }
    const auto* tracking = systems_.GetContracts().GetTrackingVolume();
    int current_id = tracking ? tracking->volume_id : vols.front().volume_id;
    const int count = static_cast<int>(vols.size());
    int next = current_id;
    for (int i = 0; i < count; ++i) {
        next += (delta >= 0 ? 1 : -1);
        if (next < 1) next = 6;
        if (next > 6) next = 1;
        if (systems_.GetContracts().IsVolumeUnlocked(next)) {
            systems_.GetContracts().SetTrackingVolume(next);
            callbacks_.push_hint("已切换追踪契约：第" + std::to_string(next) + "卷。", 2.0f);
            return;
        }
    }
}

void GameRuntime::CollectArrivedMail() {
    auto& mail_orders = world_state_.MutableMailOrders();
    const int day = world_state_.MutableClock().Day();
    int delivered = 0;
    for (std::size_t i = 0; i < mail_orders.size();) {
        if (!mail_orders[i].claimed && mail_orders[i].deliver_day <= day) {
            world_state_.MutableInventory().AddItem(mail_orders[i].item_id, mail_orders[i].count);
            if (!mail_orders[i].secondary_item_id.empty() && mail_orders[i].secondary_count > 0) {
                world_state_.MutableInventory().AddItem(mail_orders[i].secondary_item_id, mail_orders[i].secondary_count);
            }
            mail_orders[i].opened = true;
            if (!mail_orders[i].receipt_sent) {
                mail_orders[i].receipt_sent = true;
            }
            mail_orders[i].claimed = true;
            ++delivered;
            mail_orders[i] = mail_orders.back();
            mail_orders.pop_back();
            continue;
        }
        ++i;
    }
    if (delivered > 0) {
        callbacks_.push_hint("已收取到达邮件：" + std::to_string(delivered) + " 封。", 2.6f);
    } else {
        callbacks_.push_hint("暂无已到达邮件。", 2.0f);
    }
}

void GameRuntime::ToggleSpiritBeastDispatch() {
    auto& beast = world_state_.MutableSpiritBeast();
    beast.dispatched_for_pest_control = !beast.dispatched_for_pest_control;
    callbacks_.push_hint(
        beast.dispatched_for_pest_control ? "灵兽已派遣，稍后将带回协助成果。" : "灵兽已待命，会在山庄附近活动。",
        2.6f);
}

bool GameRuntime::HasPendingSkillBranchChoice() const {
    return !world_state_.GetPendingSkillBranches().empty();
}

std::string GameRuntime::PendingSkillBranchSkill() const {
    if (world_state_.GetPendingSkillBranches().empty()) {
        return {};
    }
    return world_state_.GetPendingSkillBranches().front();
}

void GameRuntime::CommitPendingSkillBranch(bool choose_a) {
    auto& pending = world_state_.MutablePendingSkillBranches();
    if (pending.empty()) {
        return;
    }
    const std::string skill = pending.front();
    const std::string branch = choose_a ? SkillBranchIdFor_(skill) : SkillBranchAltIdFor_(skill);
    world_state_.MutableSkillBranches()[skill] = branch;
    pending.erase(pending.begin());
    callbacks_.push_hint(
        "技能分支已确定：" + skill + " -> " + branch + "。", 2.6f);
}

bool GameRuntime::IsFishingQteActive() const { return state_.fishing_qte_active; }
float GameRuntime::FishingQteProgress() const { return state_.fishing_qte_progress; }
float GameRuntime::FishingQteTargetCenter() const { return state_.fishing_qte_target_center; }
float GameRuntime::FishingQteTargetWidth() const { return state_.fishing_qte_target_width; }
const std::string& GameRuntime::FishingQteLabel() const { return state_.fishing_qte_label; }

void GameRuntime::StartFishingQte(const std::string& source_label) {
    if (state_.fishing_qte_active) {
        return;
    }
    state_.fishing_qte_active = true;
    state_.fishing_qte_progress = 0.08f;
    state_.fishing_qte_velocity = 0.9f + static_cast<float>(world_state_.MutableFishingAttempts() % 3) * 0.18f;
    state_.fishing_qte_target_center =
        0.40f + static_cast<float>((world_state_.MutableFishingAttempts() + world_state_.MutableClock().Hour()) % 4) * 0.12f;
    state_.fishing_qte_target_width =
        (world_state_.MutableFishingAttempts() % 2 == 0) ? 0.22f : 0.16f;
    state_.fishing_qte_label = source_label;
    callbacks_.push_hint("垂钓中：等浮标进入亮区后按 Space / Enter 收线。", 2.0f);
}

void GameRuntime::ResolveFishingQte() {
    if (!state_.fishing_qte_active) {
        return;
    }
    const float half_width = state_.fishing_qte_target_width * 0.5f;
    const bool success = std::abs(state_.fishing_qte_progress - state_.fishing_qte_target_center) <= half_width;
    const std::string fish_id = CatchFishForSeason_(world_state_.MutableClock(), world_state_.MutableFishingAttempts());
    int fish_count = success ? 1 : 0;
    const auto it = world_state_.GetSkillBranches().find("灵钓");
    const bool tidewatch = (it != world_state_.GetSkillBranches().end() && it->second == "tidewatch");
    const bool deepcurrent = (it != world_state_.GetSkillBranches().end() && it->second == "deepcurrent");
    if (success && tidewatch) {
        fish_count = 2;
    }
    if (fish_count > 0) {
        world_state_.MutableInventory().AddItem(fish_id, fish_count);
        world_state_.MutableLastFishCatch() = fish_id;
        if (deepcurrent) {
            world_state_.MutableInventory().AddItem("fish_oil", 1);
        }
        callbacks_.push_hint(
            "收线成功：" + ItemDisplayName(fish_id) + " x" + std::to_string(fish_count) + "。", 2.2f);
    } else {
        callbacks_.push_hint("收线过早，鱼影散开了。", 1.8f);
    }
    world_state_.MutableFishingAttempts() += 1;
    state_.fishing_qte_active = false;
    state_.fishing_qte_label.clear();
}

PlacedObject* GameRuntime::FindActiveDiyPreview_() {
    auto& placed = world_state_.MutablePlacedObjects();
    for (auto it = placed.rbegin(); it != placed.rend(); ++it) {
        if (it->room == "tea_room" && it->custom_data == "preview") {
            return &(*it);
        }
    }
    return nullptr;
}

const PlacedObject* GameRuntime::FindActiveDiyPreview_() const {
    const auto& placed = world_state_.GetPlacedObjects();
    for (auto it = placed.rbegin(); it != placed.rend(); ++it) {
        if (it->room == "tea_room" && it->custom_data == "preview") {
            return &(*it);
        }
    }
    return nullptr;
}

bool GameRuntime::IsDiyPlacementActive() const { return FindActiveDiyPreview_() != nullptr; }
int GameRuntime::DiyCursorX() const { return FindActiveDiyPreview_() ? FindActiveDiyPreview_()->tile_x : 0; }
int GameRuntime::DiyCursorY() const { return FindActiveDiyPreview_() ? FindActiveDiyPreview_()->tile_y : 0; }
int GameRuntime::DiyRotation() const { return FindActiveDiyPreview_() ? FindActiveDiyPreview_()->rotation : 0; }
std::string GameRuntime::DiyPreviewObjectId() const {
    return FindActiveDiyPreview_() ? FindActiveDiyPreview_()->object_id : std::string{};
}

void GameRuntime::MoveDiyCursor(int dx, int dy) {
    if (auto* preview = FindActiveDiyPreview_(); preview != nullptr) {
        preview->tile_x = std::clamp(preview->tile_x + dx, 0, 5);
        preview->tile_y = std::clamp(preview->tile_y + dy, 0, 3);
    }
}

void GameRuntime::RotateDiyPreview() {
    if (auto* preview = FindActiveDiyPreview_(); preview != nullptr) {
        preview->rotation = (preview->rotation + 90) % 360;
    }
}

void GameRuntime::ConfirmDiyPlacement() {
    if (auto* preview = FindActiveDiyPreview_(); preview != nullptr) {
        preview->custom_data = "placed";
        callbacks_.push_hint("DIY：摆放完成，可回到装饰台继续预览下一个物件。", 2.2f);
    }
}

void GameRuntime::PickupLastDiyObject() {
    auto& placed = world_state_.MutablePlacedObjects();
    for (auto it = placed.rbegin(); it != placed.rend(); ++it) {
        if (it->room == "tea_room") {
            const std::string name = ItemDisplayName(it->object_id);
            placed.erase(std::next(it).base());
            callbacks_.push_hint("DIY：已收回摆件 " + name + "。", 2.0f);
            return;
        }
    }
}

// ============================================================================
// 【GameRuntime::SleepToNextMorning】睡眠到第二天
// ============================================================================
SleepResult GameRuntime::SleepToNextMorning() {
    SleepResult result;

    int daily_influence = 0;
    for (const auto& plot : world_state_.MutableTeaPlots()) {
        if (plot.seeded && plot.watered) daily_influence += GameConstants::Cloud::WateredPlotInfluence;
    }
    if (!world_state_.MutableMainHouseRepair().completed) daily_influence += GameConstants::Cloud::MainHouseRepairInfluence;
    if (!world_state_.MutableSpiritBeast().daily_interacted) daily_influence += GameConstants::Cloud::NoBeastInteractionPenalty;
    systems_.AddPlayerInfluence(daily_influence);

    world_state_.MutableStamina().Refill();
    world_state_.MutableBuffs().Tick(0.0f);

    TrySpiritBeastWateringAid(
        world_state_.MutableSpiritBeast(),
        world_state_.MutableSpiritBeastWateredToday(),
        world_state_.MutableTeaPlots(),
        world_state_.MutableStamina(),
        RefreshTeaPlotVisual,
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        CloudSeamanor::infrastructure::Logger::Info);

    result.spirit_gain = systems_.GetCloud().SpiritEnergyGain();
    result.contract_progress = systems_.GetContracts().CompletedVolumeCount();
    result.message = "新的一天开始了。每日行动已刷新，昨天没做完的事情今天也能继续。"
                     "灵气 +" + std::to_string(result.spirit_gain) + "。";
    if (result.contract_progress > 0) {
        result.message += " 契约进度：" + std::to_string(result.contract_progress) + "卷已完成。";
    }

    callbacks_.push_hint(result.message, GameConstants::Ui::HintDuration::Welcome);
    CloudSeamanor::infrastructure::Logger::Info("玩家睡眠到第二天。");

    // 统一日切入口：推进时钟到次日早晨后，调用 OnDayChanged 完成所有每日结算。
    const int previous_day = world_state_.MutableClock().Day();
    world_state_.MutableClock().SleepToNextMorning();
    if (world_state_.MutableClock().Day() != previous_day) {
        OnDayChanged();
    }
    // 主动睡眠后自动存档，保持旧版 DayCycleRuntime 的“睡觉即存档”体验。
    (void)SaveGameToSlot(active_save_slot_);

    return result;
}

// ============================================================================
// 【GameRuntime::CheckTutorialHints】教程提示检查
// ============================================================================
void GameRuntime::CheckTutorialHints() {
    if (modules_.tutorial) {
        modules_.tutorial->Update(0.0f);
        return;
    }

    if (!world_state_.MutableTutorial().intro_move_hint_shown
        && world_state_.GetSessionTime() > 2.5f) {
        world_state_.MutableTutorial().intro_move_hint_shown = true;
        callbacks_.push_hint(
            "使用 WASD 移动。明亮的描边表示这些对象可以交互；先熟悉山庄，不需要赶时间。",
            GameConstants::Ui::HintDuration::TutorialMove);
    }
    if (!world_state_.MutableTutorial().intro_interact_hint_shown
        && world_state_.MutableInteraction().highlighted_index >= 0) {
        world_state_.MutableTutorial().intro_interact_hint_shown = true;
        callbacks_.push_hint("按 E 与高亮对象交互。采集会消耗体力。", GameConstants::Ui::HintDuration::TutorialInteract);
    }
    if (!world_state_.MutableTutorial().intro_crop_hint_shown
        && world_state_.MutableInteraction().highlighted_plot_index >= 0) {
        world_state_.MutableTutorial().intro_crop_hint_shown = true;
        callbacks_.push_hint(
            "种植流程：翻土 -> 播种 -> 浇水 -> 等待 -> 收获。"
            "今天做不完也没关系，明天还能继续。", GameConstants::Ui::HintDuration::TutorialCrop);
    }
    if (!world_state_.MutableTutorial().intro_save_hint_shown
        && world_state_.GetSessionTime() > 18.0f) {
        world_state_.MutableTutorial().intro_save_hint_shown = true;
        callbacks_.push_hint(
            "提示：F6 保存，F9 读取；22:00 之后按 T 可以睡觉。"
            "山庄没有硬性期限，错过的内容之后还能再体验。", GameConstants::Ui::HintDuration::TutorialSave);
    }
}

// ============================================================================
// 【GameRuntime::SaveGame】保存游戏
// ============================================================================
bool GameRuntime::SaveGame() {
    return SaveGameToSlot(active_save_slot_);
}

bool GameRuntime::SaveGameToSlot(int slot_index) {
    SetActiveSaveSlot(slot_index);
    auto push_hint = [this](const std::string& msg, float dur) {
        callbacks_.push_hint(msg, dur);
    };
    if (in_battle_mode_) {
        // BE-053: 战斗中存档强制结束战斗，防止读档刷战利品。
        battle_manager_.Retreat();
        in_battle_mode_ = false;
        callbacks_.push_hint("检测到战斗中存档，已自动结算并退出战斗。", 2.6f);
    }
    const int battle_state = static_cast<int>(battle_manager_.CurrentState());
    if (!CloudSeamanor::engine::SaveGameState(
            save_path_,
            world_state_.MutableClock(),
            systems_.GetCloud(),
            world_state_.GetPlayer(),
            world_state_.MutableStamina(),
            world_state_.MutableHunger(),
            world_state_.GetBuffs(),
            world_state_.MutableMainHouseRepair(),
            world_state_.MutableTeaMachine(),
            world_state_.MutableSpiritBeast(),
            world_state_.MutableSpiritBeastWateredToday(),
            world_state_.MutableTeaPlots(),
            world_state_.MutableGold(),
            world_state_.GetPriceTable(),
            world_state_.GetMailOrders(),
            world_state_.GetInventory(),
            world_state_.MutableNpcs(),
            push_hint,
            &systems_.GetSkills(),
            &systems_.GetFestivals(),
            &systems_.GetDynamicLife(),
            &systems_.GetWorkshop(),
            &world_state_.MutableSocial().relationship,
            &world_state_.MutableDecorationScore(),
            &world_state_.MutablePetType(),
            &world_state_.MutablePetAdopted(),
            &world_state_.MutableAchievements(),
            &world_state_.MutableWeeklyBuyCount(),
            &world_state_.MutableWeeklySellCount(),
            &world_state_.MutableInnOrders(),
            &world_state_.MutableInnVisitorsToday(),
            &world_state_.MutableInnIncomeToday(),
            &world_state_.MutableInnReputation(),
            &world_state_.MutableCoopFedToday(),
            &world_state_.MutableLivestockEggsToday(),
            &world_state_.MutableLivestockMilkToday(),
            &world_state_.MutableSpiritRealmDailyMax(),
            &world_state_.MutableSpiritRealmDailyRemaining(),
            &in_battle_mode_,
            &battle_state,
            &state_.battle_available,
            &state_.battle_active_partners,
            &state_.equipped_weapon_id,
            &world_state_.MutableTutorial(),
            &world_state_.GetDiaryEntries(),
            &world_state_.GetRecipeUnlocks(),
            &world_state_.GetSkillBranches(),
            &world_state_.GetPendingSkillBranches(),
            &world_state_.GetPlacedObjects(),
            &world_state_.MutablePurifyReturnDays(),
            &world_state_.MutablePurifyReturnSpirits(),
            &world_state_.MutableFishingAttempts(),
            &world_state_.MutableLastFishCatch(),
            &systems_.GetNpcDevelopment().GetAllDevelopments())) {
        return false;
    }

    // 追加心事件完成状态
    std::vector<std::string> heart_lines;
    dialogue_manager_.SaveState(heart_lines);
    if (!heart_lines.empty()) {
        std::ofstream out(save_path_, std::ios::app);
        for (const auto& line : heart_lines) {
            out << line << '\n';
        }
    }

    // 追加主线剧情状态
    std::vector<std::string> plot_lines;
    plot_system_.SaveState(plot_lines);
    if (!plot_lines.empty()) {
        std::ofstream out(save_path_, std::ios::app);
        for (const auto& line : plot_lines) {
            out << line << '\n';
        }
    }
    {
        std::vector<std::string> fest_lines;
        world_state_.GetFestivalRuntime().AppendSaveLines(fest_lines);
        if (!fest_lines.empty()) {
            std::ofstream out(save_path_, std::ios::app);
            for (const auto& line : fest_lines) {
                out << line << '\n';
            }
        }
    }
    {
        std::unordered_set<std::string> rule_ids;
        for (const auto& [rule_id, _] : state_.mail_rule_last_trigger_day) {
            rule_ids.insert(rule_id);
        }
        for (const auto& [rule_id, _] : state_.mail_rule_last_trigger_season_key) {
            rule_ids.insert(rule_id);
        }
        for (const auto& [rule_id, _] : state_.mail_rule_triggered_once) {
            rule_ids.insert(rule_id);
        }
        if (!rule_ids.empty()) {
            std::ofstream out(save_path_, std::ios::app);
            for (const auto& rule_id : rule_ids) {
                const int last_day = state_.mail_rule_last_trigger_day.count(rule_id)
                    ? state_.mail_rule_last_trigger_day[rule_id] : -1;
                const int last_season_key = state_.mail_rule_last_trigger_season_key.count(rule_id)
                    ? state_.mail_rule_last_trigger_season_key[rule_id] : -1;
                const bool triggered_once = state_.mail_rule_triggered_once.count(rule_id)
                    ? state_.mail_rule_triggered_once[rule_id] : false;
                out << "mail_rule_state|"
                    << rule_id << "|"
                    << last_day << "|"
                    << last_season_key << "|"
                    << (triggered_once ? 1 : 0) << '\n';
            }
        }
    }

    // 追加任务技能解锁状态（quest_skill|<id>|1）
    if (!state_.unlocked_quest_skills.empty()) {
        std::ofstream out(save_path_, std::ios::app);
        for (const auto& id : state_.unlocked_quest_skills) {
            out << "quest_skill|" << id << "|1\n";
        }
    }
    RefreshSaveChecksum_(save_path_);
    CloudSeamanor::infrastructure::SaveSlotMetadata metadata;
    metadata.slot_index = active_save_slot_;
    metadata.exists = true;
    metadata.saved_at_text = world_state_.MutableClock().DateText() + " " + world_state_.MutableClock().TimeText();
    metadata.day = world_state_.MutableClock().Day();
    metadata.season_text = world_state_.MutableClock().PhaseText();
    metadata.main_plot_stage = 0;
    if (!plot_system_.CurrentChapterId().empty()) {
        const auto& chapters = plot_system_.GetAllChapters();
        for (std::size_t i = 0; i < chapters.size(); ++i) {
            if (chapters[i].id == plot_system_.CurrentChapterId()) {
                metadata.main_plot_stage = static_cast<int>(i) + 1;
                break;
            }
        }
        if (metadata.main_plot_stage == 0) {
            metadata.main_plot_stage = 1;
        }
    }
    {
        static const std::array<const char*, 6> kKeyAchievementIds{
            "first_crop", "gift_expert", "master_builder", "beast_bond_max", "tide_purifier", "fest_calendar"
        };
        int key_count = 0;
        const auto& ach = world_state_.GetAchievements();
        for (const char* id : kKeyAchievementIds) {
            const auto it = ach.find(id);
            if (it != ach.end() && it->second) {
                ++key_count;
            }
        }
        metadata.key_achievement_count = key_count;
    }
    metadata.last_battle_outcome = state_.battle_exit_by_retreat
        ? "retreat"
        : (state_.retention_weekly_battle_victories > 0 ? "victory" : "none");
    metadata.summary_text =
        "主线阶段 " + std::to_string(metadata.main_plot_stage)
        + " | 关键成就 " + std::to_string(metadata.key_achievement_count)
        + " | 战斗 " + metadata.last_battle_outcome;
    // 存档缩略图：优先从当前窗口截帧保存为 PNG（失败则留空，UI 使用占位符渲染）。
    metadata.thumbnail_path.clear();
    if (window_ != nullptr) {
        try {
            namespace fs = std::filesystem;
            const fs::path thumb_dir = fs::path("saves") / ("slot_" + std::to_string(active_save_slot_));
            fs::create_directories(thumb_dir);
            const fs::path thumb_path = thumb_dir / "thumbnail.png";

            const auto size = window_->getSize();
            sf::Texture capture;
            if (capture.resize({size.x, size.y})) {
                capture.update(*window_);
                sf::Image img = capture.copyToImage();
                // 生成 160x90 缩略图（最近邻）
                sf::Image small;
                constexpr unsigned int kW = 160;
                constexpr unsigned int kH = 90;
                small.resize({kW, kH}, sf::Color(200, 192, 168));
                for (unsigned int y = 0; y < kH; ++y) {
                    for (unsigned int x = 0; x < kW; ++x) {
                        const unsigned int src_x = static_cast<unsigned int>(
                            (static_cast<std::uint64_t>(x) * size.x) / kW);
                        const unsigned int src_y = static_cast<unsigned int>(
                            (static_cast<std::uint64_t>(y) * size.y) / kH);
                        small.setPixel({x, y}, img.getPixel({std::min(src_x, size.x - 1), std::min(src_y, size.y - 1)}));
                    }
                }
                if (small.saveToFile(thumb_path.string())) {
                    metadata.thumbnail_path = thumb_path.generic_string();
                }
            }
        } catch (const std::exception& ex) {
            CloudSeamanor::infrastructure::Logger::Warning(
                std::string("Save thumbnail capture failed: ") + ex.what());
            metadata.thumbnail_path.clear();
        }
    }
    save_slot_manager_.WriteMetadata(active_save_slot_, metadata);
    CloudSeamanor::infrastructure::Logger::LogSaveSuccess(
        "slot=" + std::to_string(active_save_slot_) + ", path=" + save_path_.string());
    callbacks_.push_hint("保存完成（槽位 " + std::to_string(active_save_slot_) + "）。", 2.0f);
    return true;
}

// ============================================================================
// 【GameRuntime::LoadGame】加载游戏
// ============================================================================
bool GameRuntime::LoadGame() {
    return LoadGameFromSlot(active_save_slot_);
}

bool GameRuntime::LoadGameFromSlot(int slot_index) {
    SetActiveSaveSlot(slot_index);
    auto push_hint = [this](const std::string& msg, float dur) {
        callbacks_.push_hint(msg, dur);
    };
    bool loaded_in_battle_mode = false;
    int loaded_battle_state = static_cast<int>(BattleState::Inactive);
    const bool ok = CloudSeamanor::engine::LoadGameState(
        save_path_,
        world_state_.MutableClock(),
        systems_.GetCloud(),
        world_state_.MutablePlayer(),
        world_state_.MutableStamina(),
        world_state_.MutableHunger(),
        world_state_.MutableBuffs(),
        world_state_.MutableMainHouseRepair(),
        world_state_.MutableTeaMachine(),
        world_state_.MutableSpiritBeast(),
        world_state_.MutableSpiritBeastWateredToday(),
        world_state_.MutableTeaPlots(),
        world_state_.MutableGold(),
        world_state_.MutablePriceTable(),
        world_state_.MutableMailOrders(),
        world_state_.MutableInventory(),
            world_state_.MutableNpcs(),
        world_state_.MutableObstacleShapes(),
        last_cloud_state_,
        [this]() {
            RefreshSpiritBeastVisual(world_state_.MutableSpiritBeast(),
                                    world_state_.MutableInteraction().spirit_beast_highlighted);
        },
        RefreshTeaPlotVisual,
        [this]() {
            CloudSeamanor::engine::UpdateHighlightedInteractable(
                world_state_.MutablePlayer(),
                world_state_.MutableInteractables(),
                world_state_.MutableTeaPlots(),
                world_state_.MutableNpcs(),
                world_state_.MutableSpiritBeast(),
                world_state_.MutableInteraction().highlighted_index,
                world_state_.MutableInteraction().highlighted_plot_index,
                world_state_.MutableInteraction().highlighted_npc_index,
                world_state_.MutableInteraction().spirit_beast_highlighted,
                RefreshTeaPlotVisual,
                RefreshSpiritBeastVisual);
        },
        [this]() { UpdateUi(0.0f); },
        push_hint,
        &systems_.GetSkills(),
        &systems_.GetFestivals(),
        &systems_.GetDynamicLife(),
        &systems_.GetWorkshop(),
        &world_state_.MutableSocial().relationship,
        &dialogue_manager_,
        &world_state_.MutableDecorationScore(),
        &world_state_.MutablePetType(),
        &world_state_.MutablePetAdopted(),
        &world_state_.MutableAchievements(),
        &world_state_.MutableWeeklyBuyCount(),
        &world_state_.MutableWeeklySellCount(),
        &world_state_.MutableInnOrders(),
        &world_state_.MutableInnVisitorsToday(),
        &world_state_.MutableInnIncomeToday(),
        &world_state_.MutableInnReputation(),
        &world_state_.MutableCoopFedToday(),
        &world_state_.MutableLivestockEggsToday(),
        &world_state_.MutableLivestockMilkToday(),
        &world_state_.MutableSpiritRealmDailyMax(),
        &world_state_.MutableSpiritRealmDailyRemaining(),
        &loaded_in_battle_mode,
        &loaded_battle_state,
        &state_.battle_available,
        &state_.battle_active_partners,
        &state_.equipped_weapon_id,
        &world_state_.MutableTutorial(),
        &world_state_.MutableDiaryEntries(),
        &world_state_.MutableRecipeUnlocks(),
        &world_state_.MutableSkillBranches(),
        &world_state_.MutablePendingSkillBranches(),
        &world_state_.MutablePlacedObjects(),
        &world_state_.MutablePurifyReturnDays(),
        &world_state_.MutablePurifyReturnSpirits(),
        &world_state_.MutableFishingAttempts(),
        &world_state_.MutableLastFishCatch(),
        &systems_.MutableNpcDevelopment().MutableAllDevelopments());
    {
        std::vector<std::string> npc_ids;
        npc_ids.reserve(world_state_.MutableNpcs().size());
        for (const auto& npc : world_state_.MutableNpcs()) {
            npc_ids.push_back(npc.id);
        }
        systems_.MutableNpcDevelopment().Initialize(npc_ids, world_state_.MutableClock().Day());
        for (auto& npc : world_state_.MutableNpcs()) {
            if (const auto* dev = systems_.GetNpcDevelopment().GetDevelopment(npc.id)) {
                npc.development_stage = dev->current_stage;
                npc.current_activity = dev->current_job.empty() ? npc.current_activity : dev->current_job;
                npc.current_location = dev->current_house_id.empty() ? npc.current_location : dev->current_house_id;
            }
        }
    }
    SyncUnlockedRecipesToWorkshop_(world_state_.GetRecipeUnlocks(), systems_.GetWorkshop());
    // 加载主线剧情状态（从存档文件末尾读取 plot 行）
    {
        std::ifstream in(save_path_);
        if (in.is_open()) {
            std::string line;
            std::vector<std::string> plot_lines;
            while (std::getline(in, line)) {
                if (line.rfind("main_plot|", 0) == 0) {
                    plot_lines.push_back(line);
                }
            }
            if (!plot_lines.empty()) {
                plot_system_.LoadState(plot_lines);
            }
        }
    }
    // 加载任务技能解锁状态
    state_.unlocked_quest_skills.clear();
    {
        std::ifstream in(save_path_);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                if (line.rfind("quest_skill|", 0) != 0) {
                    continue;
                }
                const auto fields = SplitSaveFields_(line);
                if (fields.size() >= 3 && fields[2] == "1" && !fields[1].empty()) {
                    state_.unlocked_quest_skills.insert(fields[1]);
                }
            }
        }
    }
    world_state_.MutableFestivalRuntime().Reset();
    state_.mail_rule_last_trigger_day.clear();
    state_.mail_rule_last_trigger_season_key.clear();
    state_.mail_rule_triggered_once.clear();
    {
        std::ifstream in(save_path_);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                const auto fields = SplitSaveFields_(line);
                if (fields.empty()) {
                    continue;
                }
                if (line.rfind("fest_rt|", 0) == 0) {
                    (void)world_state_.MutableFestivalRuntime().TryConsumeSaveLine(fields[0], fields);
                    continue;
                }
                if (line.rfind("mail_rule_state|", 0) == 0 && fields.size() >= 5) {
                    try {
                        const std::string& rule_id = fields[1];
                        state_.mail_rule_last_trigger_day[rule_id] = std::stoi(fields[2]);
                        state_.mail_rule_last_trigger_season_key[rule_id] = std::stoi(fields[3]);
                        state_.mail_rule_triggered_once[rule_id] = (std::stoi(fields[4]) != 0);
                    } catch (...) {
                        // ignore malformed legacy/custom rows
                    }
                }
            }
        }
    }
    // 重新初始化剧情系统引用（读档后需要刷新）
    plot_system_.SetGameClock(&world_state_.MutableClock());
    plot_system_.SetCloudSystem(&systems_.GetCloud());
    plot_system_.SetNpcHeartGetter([this](const std::string& npc_id) -> int {
        for (const auto& npc : world_state_.MutableNpcs()) {
            if (npc.id == npc_id) {
                return npc.heart_level;
            }
        }
        return 0;
    });
    SyncWorkshopByMainHouseLevel_(world_state_.MutableMainHouseRepair().level, systems_.GetWorkshop());
    SyncTeaMachineFromWorkshop_();
    world_state_.SetGreenhouseUnlocked(world_state_.MutableMainHouseRepair().level >= 3);
    if (!world_state_.GetGreenhouseUnlocked()) {
        world_state_.MutableGreenhouseTagNextPlanting() = false;
    }
    if (loaded_in_battle_mode || loaded_battle_state != static_cast<int>(BattleState::Inactive)) {
        battle_manager_.Retreat();
        in_battle_mode_ = false;
        callbacks_.push_hint("读档时检测到战斗态，已安全重置战斗状态。", 2.6f);
    }
    SyncEquippedWeaponFromSaveAndInventory_();
    SyncQuestSkills_();
    if (ok) {
        callbacks_.push_hint("读档完成（槽位 " + std::to_string(active_save_slot_) + "）。", 2.0f);
    }
    return ok;
}

bool GameRuntime::DeleteSaveSlot(int slot_index) {
    if (!save_slot_manager_.DeleteSlot(slot_index)) {
        callbacks_.push_hint("删除槽位失败（槽位 " + std::to_string(slot_index) + "）。", 2.4f);
        return false;
    }
    if (active_save_slot_ == slot_index) {
        const int fallback_slot = (slot_index == 1) ? 2 : 1;
        SetActiveSaveSlot(fallback_slot);
    }
    callbacks_.push_hint("已删除槽位 " + std::to_string(slot_index) + "。", 2.0f);
    return true;
}

bool GameRuntime::CopySaveSlot(int from_slot_index, int to_slot_index, bool overwrite_target) {
    if (!save_slot_manager_.CopySlot(from_slot_index, to_slot_index, overwrite_target)) {
        callbacks_.push_hint(
            "复制槽位失败（" + std::to_string(from_slot_index) + " -> "
                + std::to_string(to_slot_index) + "）。",
            2.6f);
        return false;
    }
    callbacks_.push_hint(
        "已复制槽位 " + std::to_string(from_slot_index) + " 到槽位 "
            + std::to_string(to_slot_index) + "。",
        2.2f);
    return true;
}

bool GameRuntime::RenameSaveSlot(int slot_index, const std::string& display_name) {
    if (!save_slot_manager_.RenameSlotDisplayName(slot_index, display_name)) {
        callbacks_.push_hint("重命名槽位失败（槽位 " + std::to_string(slot_index) + "）。", 2.4f);
        return false;
    }
    callbacks_.push_hint("槽位 " + std::to_string(slot_index) + " 名称已更新。", 2.0f);
    return true;
}

void GameRuntime::SetActiveSaveSlot(int slot_index) {
    if (!save_slot_manager_.IsValidSlot(slot_index)) {
        return;
    }
    active_save_slot_ = slot_index;
    save_path_ = save_slot_manager_.BuildSlotPath(slot_index);
}

std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata> GameRuntime::ReadSaveSlots() const {
    return save_slot_manager_.ReadAllMetadata();
}

// ============================================================================
// 【GameRuntime::ConsumeLevelUpEvent】消费升级事件
// ============================================================================
LevelUpEvent GameRuntime::ConsumeLevelUpEvent() {
    LevelUpEvent event;
    if (world_state_.GetLevelUpOverlayActive()) {
        event.triggered = true;
        event.skill_type = world_state_.GetLevelUpSkillType();
        event.skill_name = systems_.GetSkills().GetSkillName(event.skill_type);
        event.new_level = systems_.GetSkills().GetLevel(event.skill_type);
    }
    return event;
}

// ============================================================================
// 【GameRuntime::CanSleep】是否可以睡觉
// ============================================================================
bool GameRuntime::CanSleep() const {
    return world_state_.GetClock().Hour() >= 22
        || world_state_.GetClock().Hour() < 6;
}

// ============================================================================
// 【GameRuntime::GetCurrentTargetText】获取当前目标文本
// ============================================================================
std::string GameRuntime::GetCurrentTargetText() const {
    TargetHintContext ctx{
        world_state_.GetInventory(),
        systems_.GetWorkshop(),
        world_state_.GetTeaPlots(),
        world_state_.GetTeaBushes(),
        world_state_.GetNpcs(),
        world_state_.GetSpiritBeast(),
        world_state_.GetInteractables(),
        world_state_.GetSpiritBeastWateredToday(),
        world_state_.GetInteraction().highlighted_plot_index,
        world_state_.GetInteraction().highlighted_npc_index,
        world_state_.GetInteraction().spirit_beast_highlighted,
        world_state_.GetInteraction().highlighted_index,
        world_state_.GetClock().Day(),
        world_state_.GetClock().Hour(),
        world_state_.GetGold(),
        world_state_.GetMainHouseRepair(),
        world_state_.GetTeaMachine()
    };
    return BuildCurrentTargetText(ctx);
}

// ============================================================================
// 【GameRuntime::GetControlsHint】获取控制提示
// ============================================================================
std::string GameRuntime::GetControlsHint() const {
    return "WASD 移动  E 交互  I 背包  F 任务  M 地图  C 状态  F5 预报  F6 保存  F9 读取  F7/CTRL+W 工坊  F8 节日  O 商店  P 邮件  L 成就  V 图鉴  战斗: Q/W/E/R 技能 B/X 撤离";
}

std::size_t GameRuntime::GetEntityCount() const {
    return world_state_.GetTeaPlots().size()
        + world_state_.GetNpcs().size()
        + world_state_.GetInteractables().size();
}

std::size_t GameRuntime::GetPickupCount() const {
    return world_state_.GetPickups().size();
}

// ============================================================================
// 【GameRuntime::HandleGiftInteraction】处理 NPC 送礼
// ============================================================================
void GameRuntime::HandleGiftInteraction() {
    HandleInteractionCommon_(true);
}

// ============================================================================
// 【GameRuntime::HandlePrimaryInteraction】处理主交互
// ============================================================================
void GameRuntime::HandlePrimaryInteraction() {
    HandleInteractionCommon_(false);
}

void GameRuntime::RenderSceneTransition(sf::RenderWindow& window) {
    scene_transition_.Render(window);
}

PlayerInteractRuntimeContext GameRuntime::BuildInteractionContext_() {
    auto spawn_hearts = [](const CloudSeamanor::domain::Vec2f& pos,
                            std::vector<HeartParticle>& particles) {
        CloudSeamanor::engine::SpawnHeartParticles(pos, particles);
    };
    auto refresh_pickup = [](CloudSeamanor::domain::PickupDrop& pickup) {
        auto& shape = pickup.Shape();
        shape.setFillColor(PickupColorFor(pickup.ItemId()));
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(72, 48, 24));
    };

    PlayerInteractRuntimeContext ctx(
        world_state_.MutableClock(),
        systems_.GetCloud(),
        world_state_.MutableInventory(),
        world_state_.MutableStamina(),
        world_state_.MutableHunger(),
        world_state_.GetConfig().stamina_interact_cost,
        world_state_.MutableTeaPlots(),
        world_state_.MutableTeaBushes(),
        world_state_.MutableNpcs(),
        world_state_.MutableSpiritBeast(),
        world_state_.MutableSpiritBeastWateredToday(),
        world_state_.MutableHeartParticles(),
        world_state_.MutablePickups(),
        const_cast<const std::vector<CloudSeamanor::domain::Interactable>&>(world_state_.MutableInteractables()),
        world_state_.MutableMainHouseRepair(),
        world_state_.MutableTeaMachine(),
        world_state_.MutableObstacleShapes(),
        systems_.GetSkills(),
        systems_.GetDynamicLife(),
        systems_.GetWorkshop(),
        relationship_system_,
        world_state_.MutableSocial().relationship,
        systems_.GetFestivals(),
        world_state_.MutableNpcTextMappings(),
        world_state_.MutableInteraction().dialogue_engine,
        dialogue_data_root_,
        world_state_.MutableInteraction().dialogue_text,
        world_state_.MutableGold(),
        world_state_.MutablePriceTable(),
        world_state_.MutableMailOrders(),
        world_state_.MutableLastTradeQuality(),
        world_state_.MutableInSpiritRealm(),
        world_state_.GetActiveFestivalId(),
        &world_state_.MutableFestivalRuntime(),
        world_state_.MutableSpiritPlantLastHarvestHour(),
        world_state_.MutableWeeklyBuyCount(),
        world_state_.MutableWeeklySellCount(),
        world_state_.MutableDailyGeneralStoreStock(),
        world_state_.MutableInnGoldReserve(),
        world_state_.MutableCoopFedToday(),
        world_state_.MutableDecorationScore(),
        world_state_.MutableDiaryEntries(),
        world_state_.MutableSkillBranches(),
        world_state_.MutablePendingSkillBranches(),
        world_state_.MutablePlacedObjects(),
        world_state_.MutableFishingAttempts(),
        world_state_.MutableLastFishCatch(),
        world_state_.MutablePetType(),
        world_state_.MutablePetAdopted(),
        world_state_.MutableAchievements(),
        world_state_.MutableModHooks(),
        world_state_.MutableGreenhouseUnlocked(),
        world_state_.MutableGreenhouseTagNextPlanting(),
        world_state_.MutableClock().Hour(),
        [this](const std::string& npc_id) -> bool {
            return npc_delivery_.TryClaimRewards(world_state_, npc_id);
        },
        [this](bool to_spirit_realm) {
            RequestSpiritRealmTravel_(to_spirit_realm);
        },
        [this](const std::string& source_label) {
            StartFishingQte(source_label);
        },
        [this](const std::string& msg, float dur) { callbacks_.push_hint(msg, dur); },
        &CloudSeamanor::infrastructure::Logger::Info,
        [this](const std::string& id) {
            if (callbacks_.play_sfx) callbacks_.play_sfx(id);
        },
        [this]() { UpdateUi(0.0f); },
        [this]() { UpdateUi(0.0f); },
        [this]() { RefreshWindowTitle(); },
        [this](CloudSeamanor::domain::SkillType type, [[maybe_unused]] int new_level) {
            world_state_.SetLevelUpOverlayActive(true);
            world_state_.SetLevelUpOverlayTimer(GameConstants::Ui::LevelUp::OverlayDuration);
            world_state_.SetLevelUpSkillType(type);
        },
        RefreshTeaPlotVisual,
        spawn_hearts,
        refresh_pickup,
        world_state_.MutableInteraction(),
        world_state_.MutableInteraction().highlighted_npc_index,
        world_state_.MutableInteraction().highlighted_plot_index,
        world_state_.MutableInteraction().highlighted_index,
        world_state_.MutableClock().Day(),
        world_state_.MutableClock().Hour(),
        world_state_.MutableInteraction().spirit_beast_highlighted);

    ctx.dialogue_manager = &dialogue_manager_;
    ctx.dialogue_nodes = &world_state_.MutableInteraction().dialogue_nodes;
    ctx.dialogue_start_id = &world_state_.MutableInteraction().dialogue_start_id;
    ctx.player_name = "云海旅人";
    ctx.farm_name = "云海山庄";
    ctx.current_heart_event_id = world_state_.MutableInteraction().current_heart_event_id;
    ctx.current_heart_event_reward = world_state_.MutableInteraction().current_heart_event_reward;
    ctx.current_heart_event_flag = world_state_.MutableInteraction().current_heart_event_flag;
    return ctx;
}

void GameRuntime::RequestSpiritRealmTravel_(bool to_spirit_realm) {
    if (scene_transition_.IsActive()) {
        return;
    }

    const bool currently_in_spirit = world_state_.GetInSpiritRealm();
    if (currently_in_spirit == to_spirit_realm) {
        return;
    }

    if (to_spirit_realm && world_state_.GetSpiritRealmDailyRemaining() <= 0) {
        if (callbacks_.push_hint) {
            callbacks_.push_hint("今日灵界探索次数已用尽。", 2.2f);
        }
        return;
    }

    const std::string requested_map = to_spirit_realm
        ? "assets/maps/spirit_realm_layer1.tmx"
        : "assets/maps/prototype_farm.tmx";
    const std::string to_map = ResolveMapFromMods_(mod_loader_, requested_map);

    if (callbacks_.push_hint) {
        callbacks_.push_hint(to_spirit_realm ? "传送门开启……" : "正在返回主世界……", 1.8f);
    }

    scene_transition_.Start(
        0.18f,
        0.18f,
        [this, to_spirit_realm, to_map]() {
            world_state_.SetInSpiritRealm(to_spirit_realm);
            if (to_spirit_realm) {
                world_state_.SetSpiritRealmDailyRemaining(world_state_.GetSpiritRealmDailyRemaining() - 1);
            }
            BuildScene(
                tmx_map_,
                world_state_.MutableGroundTiles(),
                world_state_.MutableObstacleShapes(),
                world_state_.MutableObstacleBounds(),
                world_state_.MutableInteractables(),
                world_state_.MutablePickups(),
                const_cast<sf::FloatRect&>(world_state_.GetConfig().world_bounds),
                to_map,
                &CloudSeamanor::infrastructure::Logger::Info,
                CloudSeamanor::infrastructure::Logger::Warning);
            SyncTeaBushInteractables_();
            SyncNpcHouseVariantInteractables_();
        });
}

void GameRuntime::InitializeTeaBushes_() {
    auto& bushes = world_state_.MutableTeaBushes();
    if (!bushes.empty()) {
        return;
    }
    const auto& table = CloudSeamanor::domain::GetGlobalTeaBushTable();
    static const std::array<CloudSeamanor::domain::Vec2f, 4> kBushPositions{
        CloudSeamanor::domain::Vec2f{548.0f, 356.0f},
        CloudSeamanor::domain::Vec2f{586.0f, 356.0f},
        CloudSeamanor::domain::Vec2f{624.0f, 356.0f},
        CloudSeamanor::domain::Vec2f{662.0f, 356.0f}
    };
    std::size_t index = 0;
    for (const auto& def : table.All()) {
        if (def.unlock_condition != "default") {
            continue;
        }
        CloudSeamanor::domain::TeaBush bush;
        bush.id = def.id;
        bush.name = def.name;
        bush.tea_id = def.tea_id;
        bush.harvest_interval_days = std::max(1, def.harvest_interval_days);
        bush.last_harvest_day = world_state_.GetClock().Day() - bush.harvest_interval_days;
        bush.position = kBushPositions[index % kBushPositions.size()];
        bushes.push_back(std::move(bush));
        ++index;
    }
}

void GameRuntime::SyncTeaBushInteractables_() {
    auto& interactables = world_state_.MutableInteractables();
    interactables.erase(
        std::remove_if(
            interactables.begin(),
            interactables.end(),
            [](const CloudSeamanor::domain::Interactable& i) {
                return i.Label() == "Tea Bush";
            }),
        interactables.end());
    if (world_state_.GetInSpiritRealm()) {
        return;
    }
    for (const auto& bush : world_state_.GetTeaBushes()) {
        interactables.emplace_back(
            sf::Vector2f(bush.position.x, bush.position.y),
            sf::Vector2f(bush.size.x, bush.size.y),
            CloudSeamanor::domain::InteractableType::GatheringNode,
            "Tea Bush",
            bush.tea_id + "_leaf",
            1,
            bush.id);
    }
}

void GameRuntime::SyncNpcHouseVariantInteractables_() {
    auto& interactables = world_state_.MutableInteractables();
    interactables.erase(
        std::remove_if(
            interactables.begin(),
            interactables.end(),
            [](const CloudSeamanor::domain::Interactable& i) {
                return i.Label().rfind("NPC House Variant:", 0) == 0;
            }),
        interactables.end());
    if (world_state_.GetInSpiritRealm()) {
        return;
    }

    std::ifstream in("assets/data/npc/npc_house_variants.csv");
    if (!in.is_open()) {
        return;
    }

    std::unordered_map<int, std::size_t> stage_counts;
    for (const auto& npc : world_state_.GetNpcs()) {
        stage_counts[std::max(0, npc.development_stage)] += 1;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("HouseId,", 0) == 0) {
            continue;
        }

        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            fields.push_back(part);
        }
        if (fields.size() < 7) {
            continue;
        }

        const std::string house_id = fields[0];
        const int stage = std::max(0, std::atoi(fields[1].c_str()));
        const bool visible = std::atoi(fields[5].c_str()) != 0;
        const std::string interaction_tag = fields[6];

        if (!visible) {
            continue;
        }
        if (!stage_counts.contains(stage)) {
            continue;
        }

        const sf::Vector2f anchor = AnchorForLocation(house_id);
        interactables.emplace_back(
            anchor,
            sf::Vector2f{68.0f, 68.0f},
            CloudSeamanor::domain::InteractableType::Storage,
            "NPC House Variant:" + house_id,
            "",
            1,
            interaction_tag);
    }
}

void GameRuntime::UpdateTeaBushes_() {
    auto& bushes = world_state_.MutableTeaBushes();
    if (bushes.empty()) {
        return;
    }
    for (auto& bush : bushes) {
        ++bush.age_days;
    }
}

void GameRuntime::HandleInteractionCommon_(bool is_gift_interaction) {
    auto ctx = BuildInteractionContext_();
    if (is_gift_interaction) {
        CloudSeamanor::engine::HandleGiftInteraction(ctx);
    } else {
        CloudSeamanor::engine::HandlePrimaryInteraction(ctx);
    }
    Event ev;
    ev.type = is_gift_interaction ? "OnGiftGiven" : "OnInteract";
    ev.data["day"] = std::to_string(world_state_.MutableClock().Day());
    GlobalEventBus().Emit(ev);
}

// ============================================================================
// 【GameRuntime::RefreshWindowTitle】刷新窗口标题
// ============================================================================
void GameRuntime::RefreshWindowTitle() {
    if (!window_) return;

    CloudSeamanor::engine::RefreshWindowTitle(
        *window_,
        world_state_.MutableClock(),
        systems_.GetCloud(),
        world_state_.GetPlayer(),
        world_state_.MutableStamina(),
        world_state_.MutableMainHouseRepair(),
        world_state_.MutableTeaMachine(),
        world_state_.MutableSpiritBeast(),
        world_state_.MutableNpcs(),
        world_state_.MutablePickups(),
        world_state_.MutableInteraction().highlighted_index,
        GetCurrentTargetText(),
        CanSleep(),
        &systems_.GetSkills(),
        &systems_.GetFestivals());
}

// ============================================================================
// 【GameRuntime::UpdateCropGrowth】更新作物生长
// ============================================================================
void GameRuntime::UpdateCropGrowth(float delta_seconds) {
    const float cloud_multiplier = CloudMultiplier();
    crop_growth_.Update(world_state_, delta_seconds, systems_.GetCloud());
    tea_garden_.UpdatePlots(
        world_state_.MutableTeaGardenPlots(),
        delta_seconds * cloud_multiplier,
        systems_.GetCloud().CurrentState(),
        world_state_.MutableClock().Day());
}

// ============================================================================
// 【GameRuntime::UpdateWorkshop】更新工坊
// ============================================================================
void GameRuntime::UpdateWorkshop(float delta_seconds) {
    if (modules_.workshop) {
        modules_.workshop->Update(delta_seconds);
    }
    SyncTeaMachineFromWorkshop_();
}

// ============================================================================
// 【GameRuntime::UpdateNpcs】更新 NPC
// ============================================================================
void GameRuntime::UpdateNpcs(float delta_seconds) {
    npc_schedule_.Update(world_state_, delta_seconds);
    // B-6 NPC 互相互动：距离很近时共享活动标签。
    auto& npcs = world_state_.MutableNpcs();
    for (std::size_t i = 0; i < npcs.size(); ++i) {
        for (std::size_t j = i + 1; j < npcs.size(); ++j) {
            const sf::Vector2f d =
                CloudSeamanor::adapter::ToSf(npcs[i].position)
                - CloudSeamanor::adapter::ToSf(npcs[j].position);
            if ((d.x * d.x + d.y * d.y) < 900.0f) {
                npcs[i].current_activity = "聊天中";
                npcs[j].current_activity = "聊天中";
            }
        }
    }
}

// ============================================================================
// 【GameRuntime::UpdateSpiritBeast】更新灵兽
// ============================================================================
void GameRuntime::UpdateSpiritBeast(float delta_seconds) {
    auto& beast = world_state_.MutableSpiritBeast();

    if (beast.state == SpiritBeastState::Interact) {
        beast.interact_timer -= delta_seconds;
        if (beast.interact_timer <= 0.0f) {
            beast.state = SpiritBeastState::Follow;
        }
    } else {
        const auto player_pos_d = world_state_.GetPlayer().GetPosition();
        const sf::Vector2f player_pos(player_pos_d.x, player_pos_d.y);
        const sf::Vector2f beast_pos = CloudSeamanor::adapter::ToSf(beast.position);
        const sf::Vector2f to_player = player_pos - beast_pos;
        const float distance = std::sqrt(to_player.x * to_player.x + to_player.y * to_player.y);

        if (distance < GameConstants::SpiritBeast::FollowStartDistance) {
            beast.state = SpiritBeastState::Follow;
        } else if (distance > GameConstants::SpiritBeast::FollowStopDistance) {
            beast.state = SpiritBeastState::Wander;
        }
    }

    spirit_beast_.SetState(beast.state);
    spirit_beast_.Update(world_state_, delta_seconds);

    beast.state = spirit_beast_.CurrentState();
    RefreshSpiritBeastVisual(beast, world_state_.MutableInteraction().spirit_beast_highlighted);
}

// ============================================================================
// 【GameRuntime::UpdateParticles】更新粒子
// ============================================================================
void GameRuntime::UpdateParticles(float delta_seconds) {
    auto& particles = world_state_.MutableHeartParticles();
    while (particles.size() > static_cast<std::size_t>(GameConstants::SpiritBeast::MaxParticles)) {
        particles.pop_back();
    }

    for (std::size_t i = 0; i < particles.size();) {
        auto& p = particles[i];
        p.lifetime -= delta_seconds;
        if (p.lifetime <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }
        p.velocity.y += GameConstants::SpiritBeast::ParticleGravity * delta_seconds;
        p.position.x += p.velocity.x * delta_seconds;
        p.position.y += p.velocity.y * delta_seconds;

        const float alpha_ratio = std::max(0.0f, p.lifetime / GameConstants::SpiritBeast::ParticleLifetime);
        const std::uint8_t alpha = static_cast<std::uint8_t>(220.0f * alpha_ratio);
        p.color_rgba = (p.color_rgba & 0xFFFFFF00u) | static_cast<std::uint32_t>(alpha);
        ++i;
    }
}

// ============================================================================
// 【GameRuntime::SyncTeaMachineFromWorkshop_】同步制茶机显示状态
// ============================================================================
void GameRuntime::SyncTeaMachineFromWorkshop_() {
    auto& tea_machine = world_state_.MutableTeaMachine();

    if (const auto* machine = systems_.GetWorkshop().GetMachine("tea_machine")) {
        tea_machine.running = machine->is_processing;
        tea_machine.progress = machine->is_processing ? machine->progress : 0.0f;
        tea_machine.duration = 100.0f;
        return;
    }

    tea_machine.running = false;
    tea_machine.progress = 0.0f;
    tea_machine.duration = 100.0f;
}

// ============================================================================
// 【GameRuntime::UpdateHighlightedInteractable】更新高亮交互
// ============================================================================
void GameRuntime::UpdateHighlightedInteractable() {
    CloudSeamanor::engine::UpdateHighlightedInteractable(
        world_state_.GetPlayer(),
        world_state_.MutableInteractables(),
        world_state_.MutableTeaPlots(),
        world_state_.MutableNpcs(),
        world_state_.MutableSpiritBeast(),
        world_state_.MutableInteraction().highlighted_index,
        world_state_.MutableInteraction().highlighted_plot_index,
        world_state_.MutableInteraction().highlighted_npc_index,
        world_state_.MutableInteraction().spirit_beast_highlighted,
        RefreshTeaPlotVisual,
        RefreshSpiritBeastVisual);
}

// ============================================================================
// 【GameRuntime::UpdateUi】更新 UI
// ============================================================================
void GameRuntime::UpdateUi(float delta_seconds) {
    UpdateStaminaBar(world_state_);
    if (modules_.workshop) {
        modules_.workshop->UpdateProgressBar();
    } else {
        UpdateWorkshopProgressBar(world_state_, systems_.GetWorkshop());
    }
    UpdateWorldTipPulse(world_state_, delta_seconds);
    if (callbacks_.update_hud_text) {
        callbacks_.update_hud_text();
    }
}

}  // namespace CloudSeamanor::engine
