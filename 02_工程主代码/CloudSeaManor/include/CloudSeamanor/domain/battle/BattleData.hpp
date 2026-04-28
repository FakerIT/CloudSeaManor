#pragma once

// ============================================================================
// 【BattleData】Battle numeric & data definitions (domain)
// ============================================================================
// Responsibilities:
// - Provide data structures for battle balance, elements, weapons, quest skills,
//   and pet skills.
// - Keep pure data + minimal helpers only (no SFML dependency).
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace CloudSeamanor::domain::battle {

// ============================================================================
// 【AuraElement】云海元素体系（A2）
// ============================================================================
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

// ============================================================================
// 【BattleStats】通用战斗属性（玩家/宠物/敌人可复用）
// ============================================================================
struct BattleStats {
    float hp = 0.0f;
    float mp = 0.0f;

    float energy_max = 100.0f;
    float energy_regen_per_second = 5.0f;

    float atk = 0.0f;
    float def = 0.0f;
    float spd = 0.0f;

    float crit_chance = 0.0f;      // 0..1
    float crit_multiplier = 1.5f;  // >=1

    float purify_rate = 1.0f; // 净化倍率（对“污染值削减”的总加成）
};

// ============================================================================
// 【ElementRules】元素克制规则（数据驱动）
// ============================================================================
struct ElementRules {
    float advantage_mult = 1.5f;
    float disadvantage_mult = 0.75f;
    float neutral_mult = 1.0f;

    // Five-cycle: wind > cloud > dew > glow > tide > wind.
    std::vector<AuraElement> cycle{
        AuraElement::Wind,
        AuraElement::Cloud,
        AuraElement::Dew,
        AuraElement::Glow,
        AuraElement::Tide,
    };
};

// ============================================================================
// 【WeaponEffectType】武器特效类型（可落地到数值修正/状态）
// ============================================================================
enum class WeaponEffectType : std::uint8_t {
    None = 0,
    AddBuff,
    AddDebuff,
    Dispel,
    EnergyOnHit,
    BonusVsTaggedTarget,
};

struct WeaponEffectData {
    std::string id;
    WeaponEffectType type = WeaponEffectType::None;
    float chance = 0.0f;      // 0..1
    float value = 0.0f;       // 通用数值
    float duration = 0.0f;    // 秒
    std::string tag;          // 目标标签/状态标签
};

// ============================================================================
// 【WeaponData】武器数据（配置化）
// ============================================================================
struct WeaponData {
    std::string id;
    std::string name;
    std::string weapon_type; // tea_tool_light / tea_tool_heavy / farm_tool / ...
    AuraElement element = AuraElement::Neutral;

    float base_attack = 20.0f;
    float purify_rate_bonus = 0.0f;
    float crit_chance_bonus = 0.0f;
    float crit_multiplier_bonus = 0.0f;
    float energy_recover_bonus = 0.0f;
    float skill_cooldown_scale = 1.0f; // <1 faster
    int quality = 1;

    std::vector<WeaponEffectData> effects;
};

// ============================================================================
// 【QuestSkillData】任务技能（解锁制，非经验升级）
// ============================================================================
enum class QuestSkillCategory : std::uint8_t {
    ExploreActive = 0,
    BattleActive,
    BattlePassive,
};

enum class QuestSkillEffectType : std::uint8_t {
    None = 0,
    Tag,
    CrowdControlRoot,
    DamageReductionFirstHit,
    EnergyRegenBonus,
    MarkTarget,
};

struct QuestSkillEffectData {
    QuestSkillEffectType type = QuestSkillEffectType::None;
    float value = 0.0f;
    float duration = 0.0f;
    std::string tag;
};

struct QuestSkillUnlockCondition {
    std::string type;   // quest_completed / npc_favor_ge / plot_flag_set / ...
    std::string id;     // quest id / npc id / flag id
    int threshold = 0;  // favor/heart threshold etc.
};

struct QuestSkillData {
    std::string id;
    std::string name;
    QuestSkillCategory category = QuestSkillCategory::BattleActive;

    float energy_cost = 0.0f;
    float cooldown_seconds = 0.0f;

    std::vector<QuestSkillEffectData> effects;
    QuestSkillUnlockCondition unlock;
};

// ============================================================================
// 【PetSkillData】灵兽技能（战斗技能子集）
// ============================================================================
enum class PetSkillKind : std::uint8_t {
    ActiveCommand = 0,  // 玩家指令释放
    PassiveAura,        // 常驻光环
    AutoTrigger,        // 条件触发
};

enum class PetSkillEffectType : std::uint8_t {
    None = 0,
    Purify,
    HealEnergy,
    AddShield,
    Root,
    Slow,
    AddTag,
    ReduceResist,
};

struct PetSkillEffectData {
    PetSkillEffectType type = PetSkillEffectType::None;
    float value = 0.0f;
    float duration = 0.0f;
    AuraElement element = AuraElement::Neutral;
    std::string tag;
};

struct PetSkillTriggerCondition {
    std::string type;          // player_energy_ratio_lt / enemy_tagged / interval / ...
    float threshold = 0.0f;    // ratio threshold etc.
};

struct PetSkillData {
    std::string id;
    std::string name;
    PetSkillKind kind = PetSkillKind::ActiveCommand;

    float cooldown_seconds = 0.0f;
    float energy_cost = 0.0f; // usually 0 for pets
    float trigger_chance = 1.0f; // loyalty * trigger_chance as final probability

    PetSkillTriggerCondition trigger;
    std::vector<PetSkillEffectData> effects;
};

// ============================================================================
// 【SpiritPetData】灵兽战斗数据（与跟随系统解耦）
// ============================================================================
struct SpiritPetData {
    std::string id;
    std::string name;
    AuraElement element = AuraElement::Neutral;

    BattleStats base_stats;
    float loyalty = 0.6f; // 0..1

    std::vector<std::string> skill_ids; // references to PetSkillData ids
};

// ============================================================================
// 【BattleTuning】战斗全局调参（对齐现有 balance_table.csv）
// ============================================================================
struct BattleTuning {
    float atk_weight = 0.35f;
    float def_k = 60.0f;
    float energy_to_damage_scale = 0.5f;
};

// ============================================================================
// 【FarmingSkillKind】种植技能类型
// ============================================================================
enum class FarmingSkillKind : std::uint8_t {
    PassiveDaily = 0,   // 每日被动协助
    Watering,           // 浇水技能
    Fertilizing,         // 施肥技能
    GrowthBoost,         // 生长促进
    Quality,             // 品质提升
    Harvest,             // 收获技能
    PestControl,         // 防治技能
    Scouting,            // 侦察技能
};

// ============================================================================
// 【FarmingSkillEffectType】种植技能效果类型
// ============================================================================
enum class FarmingSkillEffectType : std::uint8_t {
    None = 0,
    WaterOnePlot,        // 浇灌一块地
    WaterAllDry,         // 浇灌所有干旱地
    FertilizerBasic,     // 基础施肥
    FertilizerPremium,   // 高级施肥
    GrowthSpeed,         // 生长加速
    QualityBonus,        // 品质加成
    HarvestBonus,        // 收获加成
    HarvestMultiplier,    // 收获倍数
    PreventPest,         // 预防虫害
    CureDisease,         // 治愈病害
    RangeBoost,          // 范围提升
    WeatherForecast,     // 天气预告
};

// ============================================================================
// 【FarmingSkillEffectData】种植技能效果数据
// ============================================================================
struct FarmingSkillEffectData {
    FarmingSkillEffectType type = FarmingSkillEffectType::None;
    float value = 0.0f;           // 效果数值
    float weather_bonus = 1.0f;    // 天气加成
    std::string applicable_crops;  // 适用作物（all/specific）
};

// ============================================================================
// 【FarmingSkillTriggerType】种植技能触发类型
// ============================================================================
enum class FarmingSkillTriggerType : std::uint8_t {
    Manual = 0,       // 手动释放
    AutoDaily,        // 每日自动
    ReadyCrops,      // 有成熟作物时
    ManualOrAuto,     // 手动或自动
    TimeDawn,         // 黎明时分
    TideWeather,      // 潮汐天气
    NightOnly,        // 仅夜晚
    Festival,         // 节日期间
    Always,           // 常驻
};

// ============================================================================
// 【FarmingSkillData】种植技能数据
// ============================================================================
struct FarmingSkillData {
    std::string id;
    std::string name;
    FarmingSkillKind kind = FarmingSkillKind::PassiveDaily;

    float cooldown_seconds = 0.0f;
    float duration = 0.0f;
    float energy_cost = 0.0f;

    FarmingSkillTriggerType trigger_type = FarmingSkillTriggerType::Manual;
    float trigger_chance = 1.0f;

    std::vector<FarmingSkillEffectData> effects;
    std::string animation;
    float personality_bonus = 1.0f;  // 性格加成倍率
};

// ============================================================================
// 【SpiritBeastPersonalityType】灵兽性格类型
// ============================================================================
enum class SpiritBeastPersonalityType : std::uint8_t {
    Lively = 0,    // 活泼好动
    Lazy = 1,      // 懒散悠闲
    Curious = 2,   // 好奇心强
};

// ============================================================================
// 【SpiritBeastBaseStats】灵兽基础属性
// ============================================================================
struct SpiritBeastBaseStats {
    float hp = 80.0f;
    float mp = 50.0f;
    float atk = 15.0f;
    float def = 10.0f;
    float spd = 30.0f;
};

// ============================================================================
// 【SpiritBeastPersonalityBonus】灵兽性格加成
// ============================================================================
struct SpiritBeastPersonalityBonus {
    float watering = 1.0f;     // 浇水加成
    float farming = 1.0f;      // 农作加成
    float combat = 1.0f;        // 战斗加成
    float healing = 1.0f;       // 治疗加成
    float buff = 1.0f;          // 增益加成
    float scouting = 1.0f;      // 侦察加成
    float quality = 1.0f;       // 品质加成
    float harvest = 1.0f;       // 收获加成
    float growth = 1.0f;        // 生长加成
    float tide = 1.0f;          // 潮汐加成
    float defense = 1.0f;       // 防御加成
    float exploration = 1.0f;    // 探索加成
};

// ============================================================================
// 【SpiritBeastBaseData】灵兽基础数据
// ============================================================================
struct SpiritBeastBaseData {
    std::string id;
    std::string name;
    std::string description;

    std::string rarity;                  // common/uncommon/rare/legendary
    SpiritBeastPersonalityType personality = SpiritBeastPersonalityType::Lively;
    AuraElement element = AuraElement::Neutral;

    SpiritBeastBaseStats base_stats;
    float loyalty_cap = 0.9f;
    float growth_rate = 1.0f;
    SpiritBeastPersonalityBonus personality_bonus;

    std::string preferred_skill_type;    // buff/purify/heal/shield
    int unlock_heart_level = 1;
    std::string base_sprite_id;
};

// ============================================================================
// 【SpiritBeastSkillLink】灵兽技能关联
// ============================================================================
struct SpiritBeastSkillLink {
    std::string spirit_beast_id;
    std::string battle_skill_id;
    std::string farming_skill_id;
    bool is_primary = false;
    int unlock_heart_level = 1;
    std::string description;
};

// ============================================================================
// 【HeartLevelData】羁绊等级数据
// ============================================================================
struct HeartLevelData {
    int level = 0;
    int min_favor = 0;
    int unlock_skills = 0;
    int unlock_abilities = 0;
    float stat_bonus = 0.0f;
    float exp_multiplier = 1.0f;
    std::string description;
};

// ============================================================================
// 【SpiritBeastRegistry】灵兽完整数据注册表
// ============================================================================
struct SpiritBeastRegistry {
    std::unordered_map<std::string, SpiritBeastBaseData> beasts_by_id;
    std::unordered_map<std::string, PetSkillData> battle_skills_by_id;
    std::unordered_map<std::string, FarmingSkillData> farming_skills_by_id;
    std::vector<SpiritBeastSkillLink> skill_links;
    std::unordered_map<int, HeartLevelData> heart_levels;  // key: level
};

}  // namespace CloudSeamanor::domain::battle

