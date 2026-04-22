#include "CloudSeamanor/engine/BattleField.hpp"

#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

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

SpiritType ParseSpiritType_(const std::string& value) {
    if (value == "elite") return SpiritType::Elite;
    if (value == "boss") return SpiritType::Boss;
    return SpiritType::Common;
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
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 7) {
            continue;
        }

        PollutedSpirit spirit;
        spirit.id = cols[0];
        spirit.name = cols[1];
        spirit.type = ParseSpiritType_(cols[2]);
        spirit.element = ParseElement_(cols[3]);
        spirit.max_pollution = std::stof(cols[4]);
        spirit.current_pollution = spirit.max_pollution;
        spirit.attack_damage = std::stof(cols[5]);
        spirit.speed = std::stof(cols[6]);
        spirit_table_[spirit.id] = spirit;
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

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 6) {
            continue;
        }

        SkillTableEntry skill;
        skill.id = cols[0];
        skill.name = cols[1];
        skill.energy_cost = std::stof(cols[2]);
        skill.cooldown = std::stof(cols[3]);
        skill.base_power = std::stof(cols[4]);
        skill.element = ParseElement_(cols[5]);
        skill_table_[skill.id] = skill;
    }

    infrastructure::Logger::Info("BattleField: 技能表加载完成, 数量=" + std::to_string(skill_table_.size()));
    return !skill_table_.empty();
}

// ============================================================================
// 【LoadZoneTableFromCsv】加载区域表
// ============================================================================
bool BattleField::LoadZoneTableFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("BattleField: 无法打开区域表: " + file_path);
        return false;
    }

    zone_table_.clear();
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 4) {
            continue;
        }

        BattleZone zone;
        zone.id = cols[0];
        zone.name = cols[1];
        zone.background_sprite_id = cols[2];
        zone.ambient_pollution_rate = std::stof(cols[3]);
        zone.is_spirit_realm = zone.id.find("spirit") != std::string::npos;
        zone_table_[zone.id] = zone;
    }

    infrastructure::Logger::Info("BattleField: 区域表加载完成, 数量=" + std::to_string(zone_table_.size()));
    return !zone_table_.empty();
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
    player_state_.purification_rate = 1.0f;
    player_state_.shield_points = 0.0f;
    player_state_.shield_duration = 0.0f;
    player_state_.dodge_chance = 0.0f;
    player_state_.crit_chance = 0.05f; // 基础5%暴击
    player_state_.active_buffs.clear();

    // TODO: 从数据表加载灵体数据（spirit_table.csv）
    // 目前使用硬编码数据，后续替换为数据驱动

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
    for (auto& buff : player_state_.active_buffs) {
        if (!buff.is_expired) {
            buff.remaining -= delta_seconds;
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

    // TODO: 从技能表查找技能数据（skill_table.csv）
    // 目前使用硬编码数据
    (void)skill_id;
    (void)target_id;
    (void)target_pos_x;
    (void)target_pos_y;

    return true;
}

// ============================================================================
// 【PartnerUpdate】灵兽伙伴AI
// ============================================================================
void BattleField::PartnerUpdate(float delta_seconds, float player_pos_x, float player_pos_y) {
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
                float damage = 25.0f * partner.purification_rate_mod * weather_multiplier_;
                float effective_damage = CalculatePurifyDamage_(
                    damage,
                    player_state_.purification_rate,
                    weather_multiplier_,
                    partner.heart_level,
                    ElementType::Neutral, // TODO: 从技能数据获取
                    target->element,
                    false
                );

                ApplySkillHit_(partner.active_skill_ids[i], *target, effective_damage, false);

                // 开始冷却
                float cooldown = partner.cooldown_total.empty() ? 15.0f : partner.cooldown_total[i];
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
        damage *= GameConstants::Battle::CritMultiplier;
    }
    return damage;
}

// ============================================================================
// 【GetElementMultiplier_】元素克制倍率
// ============================================================================
float BattleField::GetElementMultiplier_(ElementType skill, ElementType target) const {
    // 中性无克制
    if (skill == ElementType::Neutral || target == ElementType::Neutral) return 1.0f;

    // 光暗互克
    if (skill == ElementType::Light && target == ElementType::Dark) return 1.5f;
    if (skill == ElementType::Dark && target == ElementType::Light) return 1.5f;

    // 五行循环克制：水>火>金>木>土>水
    static const std::vector<ElementType> cycle = {
        ElementType::Water, ElementType::Fire, ElementType::Metal,
        ElementType::Wood, ElementType::Earth
    };

    for (size_t i = 0; i < cycle.size(); ++i) {
        if (skill == cycle[i] && target == cycle[(i + 1) % cycle.size()]) {
            return 1.5f;
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
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < (r1 + r2);
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
    (void)skill_id; // TODO: 用于查找技能特效

    // 计算精英/BOSS难度修正
    float difficulty_mod = 1.0f;
    if (spirit.type == SpiritType::Elite) difficulty_mod = 1.5f;
    if (spirit.type == SpiritType::Boss) difficulty_mod = 2.0f;

    float actual_damage = damage / difficulty_mod;
    spirit.current_pollution = std::max(0.0f, spirit.current_pollution - actual_damage);

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
    if (dist <= spirit.attack_range && spirit.attack_timer <= 0.0f) {
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
            player_state_.current_energy = std::max(0.0f, player_state_.current_energy - energy_cost);
            AddLog(spirit.name + " 释放干扰！消耗了你 " + std::to_string(static_cast<int>(energy_cost)) + " 能量。", false, false);
        }

        spirit.attack_timer = spirit.attack_cooldown;
    }
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
