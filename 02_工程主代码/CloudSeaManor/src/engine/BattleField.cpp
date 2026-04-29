#include "CloudSeamanor/engine/BattleField.hpp"

#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/engine/BattleZoneLoader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_map>

namespace CloudSeamanor::engine {

namespace {

std::vector<std::string> SplitCsvLine_(const std::string& line) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream iss(line);
    while (std::getline(iss, token, ',')) {
        out.push_back(token);
    }
    return out;
}

ElementType ParseElement_(const std::string& value) {
    if (value == "water") return ElementType::Water;
    if (value == "fire") return ElementType::Fire;
    if (value == "wood") return ElementType::Wood;
    if (value == "metal") return ElementType::Metal;
    if (value == "earth") return ElementType::Earth;
    if (value == "light") return ElementType::Light;
    if (value == "dark") return ElementType::Dark;
    return ElementType::Neutral;
}

bool TryParseFloat_(const std::string& text, float& out_value) {
    try {
        size_t consumed = 0;
        const float value = std::stof(text, &consumed);
        if (consumed == 0) {
            return false;
        }
        out_value = value;
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool LooksLikeHeaderRow_(const std::vector<std::string>& cols) {
    if (cols.empty()) {
        return true;
    }
    const std::string& first = cols[0];
    return first == "id" || first == "name" || first == "type";
}

std::unordered_map<std::string, std::size_t> BuildHeaderIndex_(
    const std::vector<std::string>& header_cols) {
    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < header_cols.size(); ++i) {
        index[header_cols[i]] = i;
    }
    return index;
}

std::string GetColByNameOrIndex_(
    const std::vector<std::string>& cols,
    const std::unordered_map<std::string, std::size_t>& header_index,
    const std::string& name,
    std::size_t fallback_index) {
    const auto it = header_index.find(name);
    if (it != header_index.end() && it->second < cols.size()) {
        return cols[it->second];
    }
    if (fallback_index < cols.size()) {
        return cols[fallback_index];
    }
    return "";
}

SpiritType ParseSpiritType_(const std::string& value) {
    if (value == "elite") return SpiritType::Elite;
    if (value == "boss") return SpiritType::Boss;
    return SpiritType::Common;
}

std::vector<std::string> SplitListBySemicolon_(const std::string& text) {
    std::vector<std::string> result;
    std::string token;
    std::istringstream ss(text);
    while (std::getline(ss, token, ';')) {
        if (!token.empty()) {
            const auto star_pos = token.find('*');
            if (star_pos != std::string::npos) {
                token = token.substr(0, star_pos);
            }
            if (!token.empty()) {
                result.push_back(token);
            }
        }
    }
    return result;
}

std::string ToLowerAscii_(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool IsCounterElement_(ElementType skill, ElementType target) {
    if (skill == ElementType::Light && target == ElementType::Dark) return true;
    if (skill == ElementType::Dark && target == ElementType::Light) return true;
    static const std::array<ElementType, 5> cycle = {
        ElementType::Earth,  // wind
        ElementType::Metal,  // cloud
        ElementType::Wood,   // dew
        ElementType::Fire,   // glow
        ElementType::Water   // tide
    };
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        if (skill == cycle[i] && target == cycle[(i + 1) % cycle.size()]) {
            return true;
        }
    }
    return false;
}

constexpr float kPlayerCollisionRadius = 22.0f;

float DebuffDurationMultiplierByWeather_(const CloudSeamanor::domain::CloudState state) {
    switch (state) {
    case CloudSeamanor::domain::CloudState::Clear:
        return 1.0f;
    case CloudSeamanor::domain::CloudState::Mist:
        return 0.9f;
    case CloudSeamanor::domain::CloudState::DenseCloud:
        return 1.2f;
    case CloudSeamanor::domain::CloudState::Tide:
        return 1.35f;
    }
    return 1.0f;
}

}  // namespace

// ============================================================================
// 【BattleField】构造函数
// ============================================================================
BattleField::BattleField()
    : is_active_(false)
    , is_victory_(false)
    , elapsed_time_(0.0f)
    , weather_multiplier_(1.0f)
    , random_engine_(std::random_device{}())
    , spirits_purified_count_(0)
    , spirits_total_count_(0) {
}

// ============================================================================
// 【LoadSpiritTableFromCsv】加载灵体表
// ============================================================================
bool BattleField::LoadSpiritTableFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("BattleField: 无法打开灵体表: " + file_path);
        return false;
    }

    spirit_table_.clear();
    monster_table_.clear();
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }
    const auto header_cols = SplitCsvLine_(line);
    const auto header_index = BuildHeaderIndex_(header_cols);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 7) {
            continue;
        }
        if (LooksLikeHeaderRow_(cols)) {
            continue;
        }

        PollutedSpirit spirit;
        MonsterTableEntry monster;
        float max_pollution = 0.0f;
        float max_health = 100.0f;
        float attack_damage = 0.0f;
        float speed = 0.0f;
        const std::string max_pollution_text =
            GetColByNameOrIndex_(cols, header_index, "pollution", 4);
        const std::string attack_damage_text =
            GetColByNameOrIndex_(cols, header_index, "attack_dmg", 5);
        const std::string speed_text =
            GetColByNameOrIndex_(cols, header_index, "speed", 6);
        if (!TryParseFloat_(max_pollution_text, max_pollution)
            || !TryParseFloat_(attack_damage_text, attack_damage)
            || !TryParseFloat_(speed_text, speed)) {
            continue;
        }
        spirit.id = GetColByNameOrIndex_(cols, header_index, "id", 0);
        spirit.name = GetColByNameOrIndex_(cols, header_index, "name", 1);
        const std::string spirit_type_text =
            GetColByNameOrIndex_(cols, header_index, "type", 2);
        spirit.type = ParseSpiritType_(spirit_type_text);
        const std::string element_text =
            GetColByNameOrIndex_(cols, header_index, "element", 3);
        spirit.element = ParseElement_(element_text);
        spirit.max_pollution = max_pollution;
        spirit.current_pollution = spirit.max_pollution;
        spirit.attack_damage = attack_damage;
        spirit.speed = speed;
        const std::string health_text = GetColByNameOrIndex_(cols, header_index, "health", 6);
        if (TryParseFloat_(health_text, max_health)) {
            spirit.max_health = max_health;
            spirit.current_health = max_health;
        }
        const std::string attack_cd_text = GetColByNameOrIndex_(cols, header_index, "attack_cd", 8);
        float attack_cd = 0.0f;
        if (TryParseFloat_(attack_cd_text, attack_cd)) {
            spirit.attack_cooldown = attack_cd;
        }
        const std::string reward_exp_text = GetColByNameOrIndex_(cols, header_index, "reward_exp", 13);
        float reward_exp = 0.0f;
        if (TryParseFloat_(reward_exp_text, reward_exp)) {
            spirit.reward_exp = reward_exp;
        }
        spirit.reward_item_ids = SplitListBySemicolon_(
            GetColByNameOrIndex_(cols, header_index, "rewards", 14));
        spirit.attack_range = 80.0f + static_cast<float>(
            std::max(0, static_cast<int>(spirit.speed / 20.0f)));
        if (spirit.id.empty() || spirit.name.empty()) {
            continue;
        }
        spirit_table_[spirit.id] = spirit;

        monster.id = spirit.id;
        monster.name = spirit.name;
        monster.zone = GetColByNameOrIndex_(cols, header_index, "zone", 2);
        monster.star = 1;
        const std::string star_text = GetColByNameOrIndex_(cols, header_index, "star", 3);
        float star_float = 0.0f;
        if (TryParseFloat_(star_text, star_float)) {
            monster.star = std::max(1, static_cast<int>(star_float));
        }
        monster.type = spirit.type;
        monster.element = spirit.element;
        monster.pollution = spirit.max_pollution;
        monster.health = spirit.max_health;
        monster.speed = spirit.speed;
        monster.attack_cooldown = spirit.attack_cooldown;
        monster.attack_damage = spirit.attack_damage;
        monster.reward_exp = spirit.reward_exp;
        monster.reward_item_ids = spirit.reward_item_ids;
        monster.behavior_type = GetColByNameOrIndex_(cols, header_index, "behavior_type", 18);
        monster_table_[monster.id] = monster;
    }

    infrastructure::Logger::Info("BattleField: 灵体表加载完成, 数量=" + std::to_string(spirit_table_.size()));
    return !spirit_table_.empty();
}

// ============================================================================
// 【LoadSkillTableFromCsv】加载技能表
// ============================================================================
bool BattleField::LoadSkillTableFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("BattleField: 无法打开技能表: " + file_path);
        return false;
    }

    skill_table_.clear();
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }
    const auto header_cols = SplitCsvLine_(line);
    const auto header_index = BuildHeaderIndex_(header_cols);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 6) {
            continue;
        }
        if (LooksLikeHeaderRow_(cols)) {
            continue;
        }

        SkillTableEntry skill;
        // Supports both legacy fixed-index schema and modern named-column schema.
        float energy_cost = 0.0f;
        float cooldown = 0.0f;
        float base_power = 0.0f;
        const std::string energy_text =
            GetColByNameOrIndex_(cols, header_index, "energy_cost", 2);
        const std::string cooldown_text =
            GetColByNameOrIndex_(cols, header_index, "cooldown", 3);
        const std::string base_power_text = header_index.contains("base_value")
            ? GetColByNameOrIndex_(cols, header_index, "base_value", 4)
            : GetColByNameOrIndex_(cols, header_index, "base_power", 4);
        const std::string element_text =
            GetColByNameOrIndex_(cols, header_index, "element", 5);
        if (!TryParseFloat_(energy_text, energy_cost)
            || !TryParseFloat_(cooldown_text, cooldown)
            || !TryParseFloat_(base_power_text, base_power)) {
            continue;
        }
        skill.id = GetColByNameOrIndex_(cols, header_index, "id", 0);
        skill.name = GetColByNameOrIndex_(cols, header_index, "name", 1);
        skill.energy_cost = energy_cost;
        skill.cooldown = cooldown;
        skill.base_power = base_power;
        skill.element = ParseElement_(element_text);
        if (skill.id.empty() || skill.name.empty()) {
            continue;
        }
        skill_table_[skill.id] = skill;
    }

    infrastructure::Logger::Info("BattleField: 技能表加载完成, 数量=" + std::to_string(skill_table_.size()));
    return !skill_table_.empty();
}

// ============================================================================
// 【LoadZoneTableFromCsv】加载区域表
// ============================================================================
bool BattleField::LoadZoneTableFromCsv(const std::string& file_path) {
    const BattleZoneLoader loader;
    return loader.LoadFromCsv(file_path, zone_table_);
}

bool BattleField::LoadWeaponTableFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("BattleField: 无法打开武器表: " + file_path);
        return false;
    }
    weapon_table_.clear();
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }
    const auto header_cols = SplitCsvLine_(line);
    const auto header_index = BuildHeaderIndex_(header_cols);
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 4 || LooksLikeHeaderRow_(cols)) {
            continue;
        }
        WeaponTableEntry weapon;
        weapon.id = GetColByNameOrIndex_(cols, header_index, "id", 0);
        weapon.name = GetColByNameOrIndex_(cols, header_index, "name", 1);
        weapon.weapon_type = GetColByNameOrIndex_(cols, header_index, "weapon_type", 2);
        weapon.element = ParseElement_(ToLowerAscii_(GetColByNameOrIndex_(cols, header_index, "element", 3)));
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "base_attack", 4), weapon.base_attack);
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "purify_rate_bonus", 5), weapon.purify_rate_bonus);
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "crit_chance_bonus", 6), weapon.crit_chance_bonus);
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "crit_multiplier_bonus", 7), weapon.crit_multiplier_bonus);
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "energy_recover_bonus", 8), weapon.energy_recover_bonus);
        (void)TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "skill_cooldown_scale", 9), weapon.skill_cooldown_scale);
        float quality_f = 1.0f;
        if (TryParseFloat_(GetColByNameOrIndex_(cols, header_index, "quality", 10), quality_f)) {
            weapon.quality = std::max(1, static_cast<int>(quality_f));
        }
        if (!weapon.id.empty()) {
            weapon_table_[weapon.id] = weapon;
        }
    }
    infrastructure::Logger::Info("BattleField: 武器表加载完成, 数量=" + std::to_string(weapon_table_.size()));
    if (equipped_weapon_id_.empty() && !weapon_table_.empty()) {
        equipped_weapon_id_ = weapon_table_.begin()->first;
    }
    return !weapon_table_.empty();
}

bool BattleField::LoadBattleTuningFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("BattleField: 无法打开战斗数值表: " + file_path);
        return false;
    }

    // defaults first
    tuning_config_ = BattleTuningConfig{};
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 2) {
            continue;
        }
        const std::string key = cols[0];
        float value = 0.0f;
        if (!TryParseFloat_(cols[1], value)) {
            continue;
        }
        if (key == "player_base_crit_chance") tuning_config_.player_base_crit_chance = value;
        else if (key == "player_base_purify_rate") tuning_config_.player_base_purify_rate = value;
        else if (key == "partner_base_skill_power") tuning_config_.partner_base_skill_power = value;
        else if (key == "partner_default_cooldown") tuning_config_.partner_default_cooldown = value;
        else if (key == "area_skill_hit_radius") tuning_config_.area_skill_hit_radius = value;
    }
    infrastructure::Logger::Info("BattleField: 战斗数值表加载完成: " + file_path);
    return true;
}

bool BattleField::LoadSeedDropTableFromCsv(const std::string& file_path) {
    if (seed_drop_table_.LoadFromCsv(file_path)) {
        infrastructure::Logger::Info("BattleField: 种子掉落表加载完成: " + file_path);
        return true;
    }
    return false;
}

bool BattleField::EquipWeapon(const std::string& weapon_id) {
    if (weapon_id.empty()) {
        return false;
    }
    const auto it = weapon_table_.find(weapon_id);
    if (it == weapon_table_.end()) {
        infrastructure::Logger::Warning("BattleField: 武器不存在，无法装备: " + weapon_id);
        return false;
    }
    equipped_weapon_id_ = weapon_id;
    if (is_active_) {
        ApplyWeaponStatsToPlayer_();
        AddLog("切换武器：" + it->second.name, true, true);
    }
    return true;
}

void BattleField::SetQuestSkills(const std::vector<std::string>& quest_skill_ids) {
    active_quest_skill_ids_ = quest_skill_ids;
    if (is_active_) {
        ResetQuestSkillRuntime_();
    }
}

// ============================================================================
// 【StartBattle】开始战斗
// ============================================================================
void BattleField::StartBattle(
    const BattleZone& zone,
    CloudSeamanor::domain::CloudState cloud_state,
    int num_common,
    int num_elite,
    const std::optional<std::string>& boss_id
) {
    current_zone_ = zone;
    cloud_state_ = cloud_state;
    spirits_.clear();
    partners_.clear();
    battle_log_.clear();
    battle_history_.clear();
    hit_effects_.clear();
    tea_combo_history_.clear();
    elapsed_time_ = 0.0f;
    spirits_purified_count_ = 0;
    spirits_total_count_ = 0;

    // 计算天气倍率
    switch (cloud_state) {
        case CloudSeamanor::domain::CloudState::Clear:
            weather_multiplier_ = 1.0f;
            break;
        case CloudSeamanor::domain::CloudState::Mist:
            weather_multiplier_ = 1.2f;
            break;
        case CloudSeamanor::domain::CloudState::DenseCloud:
            weather_multiplier_ = 1.35f;
            break;
        case CloudSeamanor::domain::CloudState::Tide:
            weather_multiplier_ = 1.5f;
            break;
    }

    // 初始化玩家战斗状态
    player_state_.max_energy = GameConstants::Battle::EnergyMax;
    player_state_.current_energy = player_state_.max_energy;
    player_state_.energy_recover_rate = GameConstants::Battle::EnergyRecoverPerSecond;
    player_state_.purification_rate = tuning_config_.player_base_purify_rate;
    player_state_.weapon_attack_power = 20.0f;
    player_state_.crit_multiplier = GameConstants::Battle::CritMultiplier;
    player_state_.skill_cooldown_scale = 1.0f;
    player_state_.equipped_weapon_id.clear();
    player_state_.shield_points = 0.0f;
    player_state_.shield_duration = 0.0f;
    player_state_.dodge_chance = 0.0f;
    player_state_.crit_chance = tuning_config_.player_base_crit_chance;
    player_state_.active_buffs.clear();
    ApplyWeaponStatsToPlayer_();
    ResetQuestSkillRuntime_();

    auto spawn_spirit = [this](const PollutedSpirit& proto, std::size_t index) {
        PollutedSpirit spirit = proto;
        spirit.current_pollution = spirit.max_pollution;
        spirit.current_health = spirit.max_health;
        spirit.is_defeated = false;
        spirit.is_escaped = false;
        spirit.is_stunned = false;
        spirit.stun_remaining = 0.0f;
        spirit.ai_state = SpiritAIState::Approach;
        spirit.attack_timer = 0.0f;
        InitializeImbalanceSegments_(spirit);
        spirit.current_phase = 1;
        spirit.phase_thresholds.clear();
        if (spirit.type == SpiritType::Boss) {
            spirit.phase_thresholds = {0.7f, 0.4f, 0.15f};
        }
        spirit.pos_x = 780.0f + static_cast<float>(index % 5) * 60.0f;
        spirit.pos_y = 220.0f + static_cast<float>(index / 5) * 70.0f;
        spirits_.push_back(spirit);
    };

    std::vector<PollutedSpirit> common_pool;
    std::vector<PollutedSpirit> elite_pool;
    for (const auto& [_, spirit] : spirit_table_) {
        if (spirit.type == SpiritType::Elite) {
            elite_pool.push_back(spirit);
        } else if (spirit.type == SpiritType::Common) {
            common_pool.push_back(spirit);
        }
    }

    std::size_t spawn_index = 0;
    for (int i = 0; i < num_common && !common_pool.empty(); ++i) {
        spawn_spirit(common_pool[static_cast<std::size_t>(i) % common_pool.size()], spawn_index++);
    }
    for (int i = 0; i < num_elite && !elite_pool.empty(); ++i) {
        spawn_spirit(elite_pool[static_cast<std::size_t>(i) % elite_pool.size()], spawn_index++);
    }
    if (boss_id.has_value()) {
        const auto it = spirit_table_.find(*boss_id);
        if (it != spirit_table_.end()) {
            spawn_spirit(it->second, spawn_index++);
        }
    }

    if (spirits_.empty()) {
        // Fallback: keep at least one common spirit so battle can proceed.
        for (const auto& [_, spirit] : spirit_table_) {
            if (spirit.type == SpiritType::Common) {
                spawn_spirit(spirit, 0);
                break;
            }
        }
    }

    // Initialize player default skill slots from table.
    player_state_.unlocked_skill_ids = {
        "player_wind_blade",
        "player_shadow_blade",
        "player_stone_shield",
        "player_cloud_domain"};
    player_state_.cooldown_total.assign(player_state_.unlocked_skill_ids.size(), 6.0f);
    player_state_.cooldown_remaining.assign(player_state_.unlocked_skill_ids.size(), 0.0f);
    for (std::size_t i = 0; i < player_state_.unlocked_skill_ids.size(); ++i) {
        const auto it = skill_table_.find(player_state_.unlocked_skill_ids[i]);
        if (it != skill_table_.end()) {
            player_state_.cooldown_total[i] = it->second.cooldown * player_state_.skill_cooldown_scale;
        }
    }
    spirits_total_count_ = static_cast<int>(spirits_.size());

    is_active_ = true;
    is_victory_ = false;

    AddLog("战斗开始！", true, true);
    AddLog("当前天气：" + GetWeatherText_() + "，净化效率 x" + std::to_string(weather_multiplier_), false, false);
}

// ============================================================================
// 【EndBattle】结束战斗
// ============================================================================
void BattleField::EndBattle() {
    is_active_ = false;
    is_victory_ = false;
    spirits_.clear();
    partners_.clear();
}

// ============================================================================
// 【Update】每帧更新
// ============================================================================
bool BattleField::Update(float delta_seconds, float player_pos_x, float player_pos_y) {
    if (!is_active_) return false;

    elapsed_time_ += delta_seconds;

    // 更新玩家能量回复
    if (player_state_.current_energy < player_state_.max_energy) {
        player_state_.current_energy = std::min(
            player_state_.max_energy,
            player_state_.current_energy + player_state_.energy_recover_rate * delta_seconds
        );
    }

    // 更新玩家护盾
    if (player_state_.shield_duration > 0.0f) {
        player_state_.shield_duration -= delta_seconds;
        if (player_state_.shield_duration <= 0.0f) {
            player_state_.shield_points = 0.0f;
        }
    }

    // 更新BUFF
    const float debuff_duration_multiplier = DebuffDurationMultiplierByWeather_(cloud_state_);
    for (auto& buff : player_state_.active_buffs) {
        if (!buff.is_expired) {
            float delta = delta_seconds;
            if (buff.is_debuff) {
                // Higher multiplier means the same debuff lasts longer under harsher weather.
                delta = delta_seconds / std::max(0.1f, debuff_duration_multiplier);
            }
            buff.remaining -= delta;
            if (buff.remaining <= 0.0f) {
                buff.is_expired = true;
            }
        }
    }
    // 移除过期BUFF
    player_state_.active_buffs.erase(
        std::remove_if(player_state_.active_buffs.begin(), player_state_.active_buffs.end(),
            [](const Buff& b) { return b.is_expired; }),
        player_state_.active_buffs.end()
    );

    // 更新玩家技能冷却
    for (auto& cd : player_state_.cooldown_remaining) {
        if (cd > 0.0f) {
            cd = std::max(0.0f, cd - delta_seconds);
        }
    }

    // 更新灵兽伙伴冷却
    for (auto& partner : partners_) {
        for (auto& cd : partner.cooldown_remaining) {
            if (cd > 0.0f) {
                cd = std::max(0.0f, cd - delta_seconds);
            }
        }
    }

    // 更新所有污染灵体
    for (auto& spirit : spirits_) {
        if (spirit.is_defeated || spirit.is_escaped) continue;

        // 晕眩状态
        if (spirit.is_stunned) {
            spirit.stun_remaining -= delta_seconds;
            if (spirit.stun_remaining <= 0.0f) {
                spirit.is_stunned = false;
            }
            continue;
        }

        // AI更新
        UpdateSpiritAI_(spirit, delta_seconds, player_pos_x, player_pos_y);

        // 位置移动
        spirit.pos_x += spirit.velocity_x * delta_seconds;
        spirit.pos_y += spirit.velocity_y * delta_seconds;

        // 攻击冷却
        if (spirit.attack_timer > 0.0f) {
            spirit.attack_timer -= delta_seconds;
        }

        // 持续污染效果（浓云海时敌人污染加深）
        if (spirit.pollution_regen_per_second > 0.0f) {
            spirit.current_pollution = std::min(
                spirit.max_pollution,
                spirit.current_pollution + spirit.pollution_regen_per_second * delta_seconds
            );
        }
    }

    for (std::size_t i = 0; i < hit_effects_.size();) {
        hit_effects_[i].remaining -= delta_seconds;
        if (hit_effects_[i].remaining <= 0.0f) {
            hit_effects_[i] = hit_effects_.back();
            hit_effects_.pop_back();
            continue;
        }
        ++i;
    }

    // 胜负判定
    CheckVictoryCondition_();

    return is_active_;
}

// ============================================================================
// 【PlayerCastSkill】玩家释放技能
// ============================================================================
bool BattleField::PlayerCastSkill(
    const std::string& skill_id,
    const std::optional<std::string>& target_id,
    float target_pos_x,
    float target_pos_y
) {
    if (!is_active_) return false;
    const auto skill_it = skill_table_.find(skill_id);
    if (skill_it == skill_table_.end()) {
        return false;
    }
    const auto& skill = skill_it->second;

    if (player_state_.current_energy < skill.energy_cost) {
        AddLog("能量不足，无法释放 " + skill.name + "。", true, false);
        return false;
    }

    std::size_t slot_index = player_state_.unlocked_skill_ids.size();
    for (std::size_t i = 0; i < player_state_.unlocked_skill_ids.size(); ++i) {
        if (player_state_.unlocked_skill_ids[i] == skill_id) {
            slot_index = i;
            break;
        }
    }
    if (slot_index < player_state_.cooldown_remaining.size()
        && player_state_.cooldown_remaining[slot_index] > 0.0f) {
        return false;
    }

    player_state_.current_energy -= skill.energy_cost;
    if (slot_index < player_state_.cooldown_remaining.size()) {
        player_state_.cooldown_remaining[slot_index] = skill.cooldown;
    }

    bool hit_any = false;
    auto apply_damage = [&](PollutedSpirit& spirit) {
        if (spirit.is_defeated || spirit.is_escaped) return;
        const bool is_crit = RollCritical_();
        const float damage = CalculatePurifyDamage_(
            skill.base_power + player_state_.weapon_attack_power,
            player_state_.purification_rate,
            weather_multiplier_,
            1,
            skill.element,
            spirit.element,
            is_crit);
        ApplySkillHit_(skill_id, spirit, damage, is_crit);
        hit_any = true;
    };

    if (target_id.has_value()) {
        auto it = std::find_if(spirits_.begin(), spirits_.end(),
            [&](const PollutedSpirit& spirit) { return spirit.id == *target_id; });
        if (it != spirits_.end()) {
            apply_damage(*it);
        }
    } else {
        for (auto& spirit : spirits_) {
            if (CircleCollision_(
                target_pos_x, target_pos_y, tuning_config_.area_skill_hit_radius,
                spirit.pos_x, spirit.pos_y, spirit.size_radius)) {
                apply_damage(spirit);
            }
        }
    }

    if (hit_any) {
        AddLog("施放技能：" + skill.name, true, false);
        PushTeaAction_(skill_id);
        TryTriggerTeaCombo_();
    }
    return hit_any;
}

// ============================================================================
// 【PartnerUpdate】灵兽伙伴AI
// ============================================================================
void BattleField::PartnerUpdate(float delta_seconds, float player_pos_x, float player_pos_y) {
    (void)delta_seconds;
    (void)player_pos_x;
    (void)player_pos_y;
    for (auto& partner : partners_) {
        // 遍历伙伴的所有技能，找一个可用的释放
        for (size_t i = 0; i < partner.active_skill_ids.size(); ++i) {
            if (partner.cooldown_remaining[i] > 0.0f) continue;

            // AI决策：优先攻击污染值最高的敌人
            PollutedSpirit* target = nullptr;
            float highest_pollution = -1.0f;
            for (auto& spirit : spirits_) {
                if (spirit.is_defeated || spirit.is_escaped) continue;
                if (spirit.current_pollution > highest_pollution) {
                    highest_pollution = spirit.current_pollution;
                    target = &spirit;
                }
            }

            if (target != nullptr) {
                // 找到目标，释放技能（简化版：直接削减污染值）
                float damage = tuning_config_.partner_base_skill_power
                    * partner.purification_rate_mod * weather_multiplier_;
                const ElementType element = [&]() {
                    const auto it = skill_table_.find(partner.active_skill_ids[i]);
                    if (it != skill_table_.end()) {
                        return it->second.element;
                    }
                    return ElementType::Neutral;
                }();
                float effective_damage = CalculatePurifyDamage_(
                    damage,
                    player_state_.purification_rate,
                    weather_multiplier_,
                    partner.heart_level,
                    element,
                    target->element,
                    false
                );

                ApplySkillHit_(partner.active_skill_ids[i], *target, effective_damage, false);

                // 开始冷却
                float cooldown = partner.cooldown_total.empty()
                    ? tuning_config_.partner_default_cooldown
                    : partner.cooldown_total[i];
                if (i < partner.cooldown_remaining.size()) {
                    partner.cooldown_remaining[i] = cooldown;
                }

                AddLog(partner.name + " 对 " + target->name + " 发动净化！-" +
                       std::to_string(static_cast<int>(effective_damage)) + "%", true, false);

                break; // 每帧只释放一个技能
            }
        }
    }
}

// ============================================================================
// 【AddPartner】添加灵兽伙伴
// ============================================================================
void BattleField::AddPartner(const BattlePartner& partner) {
    partners_.push_back(partner);
    // 初始化冷却数组
    if (partners_.back().cooldown_remaining.empty()) {
        partners_.back().cooldown_remaining.assign(
            partners_.back().cooldown_total.begin(),
            partners_.back().cooldown_total.end()
        );
    }
}

// ============================================================================
// 【RemovePartner】移除灵兽伙伴
// ============================================================================
void BattleField::RemovePartner(const std::string& spirit_beast_id) {
    partners_.erase(
        std::remove_if(partners_.begin(), partners_.end(),
            [&spirit_beast_id](const BattlePartner& p) {
                return p.spirit_beast_id == spirit_beast_id;
            }),
        partners_.end()
    );
}

// ============================================================================
// 【AddLog】添加战斗日志
// ============================================================================
void BattleField::AddLog(const std::string& message, bool is_player_action, bool important) {
    BattleLogEntry entry;
    entry.timestamp = elapsed_time_;
    entry.message = message;
    entry.is_player_action = is_player_action;
    entry.is_important = important;
    battle_log_.push_back(entry);

    // 限制日志数量
    if (battle_log_.size() > 50) {
        battle_log_.erase(battle_log_.begin());
    }
}

// ============================================================================
// 【ClearLog】清空日志
// ============================================================================
void BattleField::ClearLog() {
    battle_log_.clear();
}

// ============================================================================
// 【CalculatePurifyDamage_】计算净化伤害
// ============================================================================
float BattleField::CalculatePurifyDamage_(
    float base_damage,
    float purify_rate,
    float weather_mult,
    int heart_level,
    ElementType skill_element,
    ElementType target_element,
    bool is_crit
) const {
    float damage = base_damage;
    damage *= purify_rate;
    damage *= weather_mult;
    damage *= (1.0f + heart_level * 0.1f); // 羁绊加成
    damage *= GetElementMultiplier_(skill_element, target_element);
    if (is_crit) {
        damage *= player_state_.crit_multiplier;
    }
    return damage;
}

// ============================================================================
// 【GetElementMultiplier_】元素克制倍率
// ============================================================================
float BattleField::GetElementMultiplier_(ElementType skill, ElementType target) const {
    // Dual-track element system:
    // - Keep legacy ElementType for existing CSV tables and skills.
    // - Map to AuraElement (cloud/wind/dew/glow/tide + light/dark) and apply new cycle.
    //
    // wind > cloud > dew > glow > tide > wind
    // light <-> dark
    enum class AuraElement : std::uint8_t {
        Neutral = 0,
        Cloud,
        Wind,
        Dew,
        Glow,
        Tide,
        Light,
        Dark,
    };

    auto to_aura = [](ElementType e) -> AuraElement {
        switch (e) {
        case ElementType::Neutral: return AuraElement::Neutral;
        case ElementType::Light:   return AuraElement::Light;
        case ElementType::Dark:    return AuraElement::Dark;
        case ElementType::Metal:   return AuraElement::Cloud; // legacy -> aura (temporary)
        case ElementType::Earth:   return AuraElement::Wind;
        case ElementType::Wood:    return AuraElement::Dew;
        case ElementType::Fire:    return AuraElement::Glow;
        case ElementType::Water:   return AuraElement::Tide;
        }
        return AuraElement::Neutral;
    };

    const AuraElement s = to_aura(skill);
    const AuraElement t = to_aura(target);
    if (s == AuraElement::Neutral || t == AuraElement::Neutral) {
        return 1.0f;
    }
    if (s == AuraElement::Light && t == AuraElement::Dark) return 1.5f;
    if (s == AuraElement::Dark && t == AuraElement::Light) return 1.5f;

    static const std::vector<AuraElement> cycle = {
        AuraElement::Wind, AuraElement::Cloud, AuraElement::Dew,
        AuraElement::Glow, AuraElement::Tide
    };
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        if (s == cycle[i] && t == cycle[(i + 1) % cycle.size()]) {
            return 1.5f;
        }
        if (t == cycle[i] && s == cycle[(i + 1) % cycle.size()]) {
            return 0.75f;
        }
    }
    return 1.0f;
}

// ============================================================================
// 【CircleCollision_】圆形碰撞检测
// ============================================================================
bool BattleField::CircleCollision_(
    float x1, float y1, float r1,
    float x2, float y2, float r2
) const {
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float radius_sum = std::max(0.0f, r1) + std::max(0.0f, r2);
    return (dx * dx + dy * dy) <= (radius_sum * radius_sum);
}

// ============================================================================
// 【ApplySkillHit_】应用技能命中效果
// ============================================================================
void BattleField::ApplySkillHit_(
    const std::string& skill_id,
    PollutedSpirit& spirit,
    float damage,
    bool is_crit
) {
    (void)is_crit;

    std::uint32_t effect_color = 0xFFDC8CF0u;
    if (skill_id.find("shadow") != std::string::npos) {
        effect_color = 0x9678DCF0u;
    } else if (skill_id.find("wind") != std::string::npos) {
        effect_color = 0x82F0DCF0u;
    } else if (skill_id.find("stone") != std::string::npos) {
        effect_color = 0xD2B482F0u;
    }
    BattleHitEffect fx;
    fx.x = spirit.pos_x;
    fx.y = spirit.pos_y;
    fx.radius = spirit.size_radius + 6.0f;
    fx.remaining = 0.28f;
    fx.total = 0.28f;
    fx.color_rgba = effect_color;
    hit_effects_.push_back(fx);

    const ElementType skill_element = [&]() {
        const auto it = skill_table_.find(skill_id);
        if (it != skill_table_.end()) {
            return it->second.element;
        }
        return ElementType::Neutral;
    }();

    // 同元素反噬：命中任何活跃同元素段位都会额外消耗玩家能量并抬升污染。
    bool resonance_backfire = false;
    bool counter_hit_segment = false;
    for (auto& seg : spirit.imbalance_segments) {
        if (!seg.is_active) {
            continue;
        }
        if (skill_element != ElementType::Neutral && skill_element == seg.element) {
            resonance_backfire = true;
        }
        if (IsCounterElement_(skill_element, seg.element)) {
            counter_hit_segment = true;
            seg.durability = std::max(0, seg.durability - 1);
            if (seg.durability == 0 && seg.is_active) {
                seg.is_active = false;
                AddLog("✦ " + spirit.name + " 的失衡段位被调和。", true, false);
            }
            break;
        }
    }
    if (resonance_backfire) {
        const float backlash = std::max(3.0f, damage * 0.15f);
        player_state_.current_energy = std::max(0.0f, player_state_.current_energy - backlash);
        spirit.current_pollution = std::min(spirit.max_pollution, spirit.current_pollution + damage * 0.06f);
        AddLog("⚠ 灵气反噬！同元素共鸣导致你额外损失能量。", false, true);
    }

    // 计算精英/BOSS难度修正
    float difficulty_mod = 1.0f;
    if (spirit.type == SpiritType::Elite) difficulty_mod = 1.5f;
    if (spirit.type == SpiritType::Boss) difficulty_mod = 2.0f;

    float actual_damage = damage / difficulty_mod;
    if (!counter_hit_segment) {
        actual_damage *= 0.6f;
    }
    spirit.current_pollution = std::max(0.0f, spirit.current_pollution - actual_damage);

    bool all_segments_cleared = !spirit.imbalance_segments.empty();
    for (const auto& seg : spirit.imbalance_segments) {
        if (seg.is_active) {
            all_segments_cleared = false;
            break;
        }
    }
    if (all_segments_cleared) {
        spirit.current_pollution = 0.0f;
    }

    // 检查是否净化完成
    if (spirit.current_pollution <= 0.0f) {
        spirit.is_defeated = true;
        spirits_purified_count_++;
        AddLog("✓ " + spirit.name + " 已净化！", true, true);
    }
}

// ============================================================================
// 【CheckVictoryCondition_】胜负判定
// ============================================================================
void BattleField::CheckVictoryCondition_() {
    // 检查是否所有灵体都已净化
    bool all_defeated = true;
    for (const auto& spirit : spirits_) {
        if (!spirit.is_defeated && !spirit.is_escaped) {
            all_defeated = false;
            break;
        }
    }

    if (all_defeated && !spirits_.empty()) {
        is_victory_ = true;
        is_active_ = false;
        CalculateRewards_();
        AddLog("★★ 战斗胜利！★★", true, true);
    }
}

// ============================================================================
// 【UpdateSpiritAI_】敌人AI更新
// ============================================================================
void BattleField::UpdateSpiritAI_(PollutedSpirit& spirit, float delta, float player_x, float player_y) {
    (void)delta;
    // 方向朝向玩家
    float dx = player_x - spirit.pos_x;
    float dy = player_y - spirit.pos_y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 1.0f) {
        float dir_x = dx / dist;
        float dir_y = dy / dist;

        float target_speed = spirit.speed;
        if (cloud_state_ == CloudSeamanor::domain::CloudState::DenseCloud) {
            target_speed *= 0.8f; // 浓云海降低移动速度
        }

        spirit.velocity_x = dir_x * target_speed;
        spirit.velocity_y = dir_y * target_speed;
    } else {
        spirit.velocity_x = 0.0f;
        spirit.velocity_y = 0.0f;
    }

    // 攻击判定（接近玩家后攻击）
    if (CircleCollision_(
            spirit.pos_x, spirit.pos_y, spirit.attack_range,
            player_x, player_y, kPlayerCollisionRadius)
        && spirit.attack_timer <= 0.0f) {
        // 对玩家造成干扰
        float actual_damage = spirit.attack_damage;
        if (player_state_.shield_points > 0.0f) {
            float absorbed = std::min(player_state_.shield_points, actual_damage);
            player_state_.shield_points -= absorbed;
            actual_damage -= absorbed;
            AddLog(spirit.name + " 的攻击被护盾吸收了 " + std::to_string(static_cast<int>(absorbed)) + "！", true, false);
        }

        if (actual_damage > 0.0f) {
            // 干扰伤害转化为能量消耗（不扣血）
            float energy_cost = actual_damage * 0.5f;
            if (quest_first_hit_reduction_available_ && HasQuestSkill_("qs_serene_ritual")) {
                energy_cost *= 0.8f;
                quest_first_hit_reduction_available_ = false;
                AddLog("【茶道·宁心】本场首次受击减免生效。", true, false);
            }
            player_state_.current_energy = std::max(0.0f, player_state_.current_energy - energy_cost);
            AddLog(spirit.name + " 释放干扰！消耗了你 " + std::to_string(static_cast<int>(energy_cost)) + " 能量。", false, false);
        }

        spirit.attack_timer = spirit.attack_cooldown;
    }

    if (spirit.type == SpiritType::Boss && !spirit.phase_thresholds.empty() && spirit.max_pollution > 0.0f) {
        const float ratio = spirit.current_pollution / spirit.max_pollution;
        while (spirit.current_phase <= static_cast<int>(spirit.phase_thresholds.size())
            && ratio <= spirit.phase_thresholds[static_cast<std::size_t>(spirit.current_phase - 1)]) {
            ++spirit.current_phase;
            spirit.speed *= 1.12f;
            spirit.attack_damage *= 1.15f;
            spirit.attack_cooldown = std::max(1.0f, spirit.attack_cooldown * 0.92f);
            AddLog("⚠ " + spirit.name + " 进入第 " + std::to_string(spirit.current_phase) + " 阶段！", false, true);
        }
    }
}

void BattleField::InitializeImbalanceSegments_(PollutedSpirit& spirit) {
    spirit.imbalance_segments.clear();
    std::array<ElementType, 5> pool = {
        ElementType::Metal, // cloud
        ElementType::Earth, // wind
        ElementType::Wood,  // dew
        ElementType::Fire,  // glow
        ElementType::Water  // tide
    };
    std::shuffle(pool.begin(), pool.end(), random_engine_);
    const std::size_t count = 4 + static_cast<std::size_t>(random_engine_() % 2);
    for (std::size_t i = 0; i < count; ++i) {
        ImbalanceSegment seg;
        seg.element = pool[i];
        seg.durability = 1;
        seg.is_active = true;
        spirit.imbalance_segments.push_back(seg);
    }
}

void BattleField::PushTeaAction_(const std::string& skill_id) {
    tea_combo_history_.push_back(skill_id);
    while (tea_combo_history_.size() > 3) {
        tea_combo_history_.pop_front();
    }
}

void BattleField::TryTriggerTeaCombo_() {
    static const std::array<const char*, 3> kTeaCombo = {
        "player_stone_shield",
        "player_cloud_domain",
        "player_wind_blade",
    };
    if (tea_combo_history_.size() != kTeaCombo.size()) {
        return;
    }
    for (std::size_t i = 0; i < kTeaCombo.size(); ++i) {
        if (tea_combo_history_[i] != kTeaCombo[i]) {
            return;
        }
    }
    int burst_count = 0;
    for (auto& spirit : spirits_) {
        if (spirit.is_defeated || spirit.is_escaped) {
            continue;
        }
        for (auto& seg : spirit.imbalance_segments) {
            if (!seg.is_active) {
                continue;
            }
            seg.is_active = false;
            seg.durability = 0;
            spirit.current_pollution = std::max(0.0f, spirit.current_pollution - 18.0f);
            ++burst_count;
            if (burst_count >= 2) {
                break;
            }
        }
        if (burst_count >= 2) {
            break;
        }
    }
    if (burst_count > 0) {
        AddLog("✦ 茶道连携触发：额外调和 " + std::to_string(burst_count) + " 段失衡。", true, true);
    }
    tea_combo_history_.clear();
}

bool BattleField::HasQuestSkill_(const std::string& id) const {
    for (const auto& s : active_quest_skill_ids_) {
        if (s == id) {
            return true;
        }
    }
    return false;
}

void BattleField::ResetQuestSkillRuntime_() {
    quest_first_hit_reduction_available_ = HasQuestSkill_("qs_serene_ritual");
}

// ============================================================================
// 【CalculateRewards_】计算奖励
// ============================================================================
void BattleField::CalculateRewards_() {
    result_.victory = true;
    result_.spirits_purified = spirits_purified_count_;
    result_.spirits_total = spirits_total_count_;
    result_.total_exp_gained = 0.0f;
    result_.battle_duration = elapsed_time_;

    for (const auto& spirit : spirits_) {
        if (spirit.is_defeated) {
            result_.total_exp_gained += spirit.reward_exp;
            result_.items_gained.push_back("spirit_crystal");

            // 发放物品奖励
            for (const auto& item_id : spirit.reward_item_ids) {
                result_.items_gained.push_back(item_id);
            }
        }
    }

    // 灵兽羁绊加成
    for (const auto& partner : partners_) {
        float favor_gain = 10.0f + partner.heart_level * 2.0f;
        result_.partner_favor_gained.emplace_back(partner.spirit_beast_id, static_cast<int>(favor_gain));
    }

    // 种子掉落
    RollSeedDrops_();
    for (const auto& seed_id : rolled_seed_drops_) {
        result_.items_gained.push_back(seed_id);
    }
}

// ============================================================================
// 【RollSeedDrops_】掷骰种子掉落
// ============================================================================
void BattleField::RollSeedDrops_() {
    rolled_seed_drops_.clear();

    if (!seed_drop_table_.IsLoaded()) {
        return;
    }

    // 遍历所有被净化的敌人，根据其星数计算掉落
    int highest_star = 0;
    for (const auto& spirit : spirits_) {
        if (spirit.is_defeated) {
            const auto it = monster_table_.find(spirit.id);
            if (it != monster_table_.end()) {
                highest_star = std::max(highest_star, it->second.star);
            }
        }
    }

    // 获取符合条件的种子掉落
    const auto& drops = seed_drop_table_.GetBattleDrops(highest_star);
    for (const auto* drop : drops) {
        const float roll = random_unit_(random_engine_);
        if (roll < drop->drop_rate) {
            rolled_seed_drops_.push_back(drop->seed_item_id);
        }
    }
}

void BattleField::ApplyWeaponStatsToPlayer_() {
    player_state_.equipped_weapon_id.clear();
    player_state_.purification_rate = tuning_config_.player_base_purify_rate;
    player_state_.crit_chance = tuning_config_.player_base_crit_chance;
    player_state_.energy_recover_rate = GameConstants::Battle::EnergyRecoverPerSecond;
    player_state_.weapon_attack_power = 20.0f;
    player_state_.crit_multiplier = GameConstants::Battle::CritMultiplier;
    player_state_.skill_cooldown_scale = 1.0f;

    if (weapon_table_.empty()) {
        return;
    }
    if (equipped_weapon_id_.empty() || !weapon_table_.contains(equipped_weapon_id_)) {
        equipped_weapon_id_ = weapon_table_.begin()->first;
    }
    const auto it = weapon_table_.find(equipped_weapon_id_);
    if (it == weapon_table_.end()) {
        return;
    }
    const auto& weapon = it->second;
    player_state_.equipped_weapon_id = weapon.id;
    player_state_.weapon_attack_power = weapon.base_attack;
    player_state_.purification_rate += weapon.purify_rate_bonus;
    player_state_.crit_chance += weapon.crit_chance_bonus;
    player_state_.crit_multiplier += weapon.crit_multiplier_bonus;
    player_state_.energy_recover_rate += weapon.energy_recover_bonus;
    player_state_.skill_cooldown_scale = std::max(0.2f, weapon.skill_cooldown_scale);
    player_state_.crit_chance = std::clamp(player_state_.crit_chance, 0.0f, 0.95f);

    for (std::size_t i = 0; i < player_state_.unlocked_skill_ids.size(); ++i) {
        const auto skill_it = skill_table_.find(player_state_.unlocked_skill_ids[i]);
        if (skill_it == skill_table_.end()) {
            continue;
        }
        if (i >= player_state_.cooldown_total.size()) {
            player_state_.cooldown_total.push_back(skill_it->second.cooldown * player_state_.skill_cooldown_scale);
            player_state_.cooldown_remaining.push_back(0.0f);
            continue;
        }
        const float old_total = std::max(0.01f, player_state_.cooldown_total[i]);
        const float old_remaining = i < player_state_.cooldown_remaining.size()
            ? player_state_.cooldown_remaining[i] : 0.0f;
        const float ratio = std::clamp(old_remaining / old_total, 0.0f, 1.0f);
        const float new_total = skill_it->second.cooldown * player_state_.skill_cooldown_scale;
        player_state_.cooldown_total[i] = new_total;
        if (i < player_state_.cooldown_remaining.size()) {
            player_state_.cooldown_remaining[i] = new_total * ratio;
        }
    }
}

bool BattleField::RollCritical_() {
    const float roll = random_unit_(random_engine_);
    return roll < player_state_.crit_chance;
}

// ============================================================================
// 【GetWeatherText_】获取天气文本
// ============================================================================
std::string BattleField::GetWeatherText_() const {
    switch (cloud_state_) {
        case CloudSeamanor::domain::CloudState::Clear: return "晴";
        case CloudSeamanor::domain::CloudState::Mist: return "薄雾";
        case CloudSeamanor::domain::CloudState::DenseCloud: return "浓云海";
        case CloudSeamanor::domain::CloudState::Tide: return "大潮";
    }
    return "晴";
}

}  // namespace CloudSeamanor::engine
