#pragma once

// ============================================================================
// 【BattleEntities.hpp】战斗实体定义
// ============================================================================
// 定义所有战斗相关的实体结构：污染灵体、战场伙伴、玩家战斗状态、增益效果。
//
// 主要类型：
// - PollutedSpirit：污染灵体（敌人）
// - BattlePartner：战场灵兽伙伴
// - BattlePlayerState：玩家战斗状态
// - Buff：增益/减益效果
// - BattleAction：战斗行动记录
// - BattleLogEntry：战斗日志条目
//
// 设计原则：
// - 仅存放数据结构和枚举，不放实现。
// - 所有类型均可拷贝/移动，支持存档序列化。
// - 与 CloudSystem/SkillSystem/SpiritBeastSystem 深度联动。
// ============================================================================

#include <string>
#include <vector>
#include <cstdint>

namespace CloudSeamanor::engine {

// ============================================================================
// 【BattleState】战斗状态
// ============================================================================
enum class BattleState {
    Inactive,  // 未在战斗中
    Loading,   // 战斗加载中
    Active,    // 战斗中
    Victory,   // 胜利结算
    Retreat,   // 撤退（无惩罚）
};

// ============================================================================
// 【SpiritType】污染灵体类型
// ============================================================================
enum class SpiritType {
    Common,  // 普通灵体：污染值100%
    Elite,   // 精英灵体：污染值200%，更强行为
    Boss,    // BOSS灵体：污染值300%+，多阶段
};

// ============================================================================
// 【SpiritAIState】污染灵体AI状态
// ============================================================================
enum class SpiritAIState {
    Idle,     // 待机
    Approach, // 接近玩家
    Attack,   // 发动攻击
    Retreat,  // 退散（被净化）
    Stunned,  // 被控制
    Confused, // 被迷惑
};

// ============================================================================
// 【ElementType】元素属性
// ============================================================================
// 克制关系：水>火>金>木>土>水，光克制暗，暗克制光，中性无克制
enum class ElementType {
    Neutral, // 中性
    Water,   // 水
    Fire,    // 火
    Wood,    // 木
    Metal,   // 金
    Earth,   // 土
    Light,   // 光
    Dark,    // 暗
};

// ============================================================================
// 【SkillType】战斗技能类型
// ============================================================================
enum class BattleSkillType {
    Passive,    // 被动（常驻）
    Active,     // 主动
    Ultimate,    // 终阶
};

// ============================================================================
// 【SkillEffect】技能主效果类型
// ============================================================================
enum class SkillEffect {
    Purify,         // 净化（削减污染值）
    Heal,           // 治疗
    Shield,         // 护盾
    Buff,           // 增益
    CrowdControl,    // 群体控制
    Debuff,         // 减益
    Dodge,          // 闪避
    EnergyRecover,   // 能量回复
};

// ============================================================================
// 【ActionType】战斗行动类型
// ============================================================================
enum class ActionType {
    PlayerSkill,     // 玩家释放技能
    PartnerSkill,    // 灵兽伙伴释放技能
    EnemyAttack,     // 敌人攻击
    EnemyPurify,    // 敌人污染加深
    BuffTick,       // BUFF回合生效
    ShieldBreak,    // 护盾破碎
    SpiritDefeated, // 灵体净化完成
    Victory,         // 战斗胜利
    Retreat,         // 撤退
};

// ============================================================================
// 【Buff】增益/减益效果
// ============================================================================
struct Buff {
    std::string id;
    std::string name;
    float duration = 0.0f;          // 持续时间（秒）
    float remaining = 0.0f;          // 剩余时间
    float purify_bonus = 0.0f;       // 净化效率加成（叠加）
    float damage_reduction = 0.0f;  // 伤害减免（0.0~1.0）
    float energy_cost_reduction = 0.0f; // 能量消耗降低
    float cooldown_reduction = 0.0f; // 冷却缩减
    float move_speed_bonus = 0.0f;  // 移动速度加成
    float dodge_bonus = 0.0f;       // 闪避率加成
    bool is_debuff = false;          // 是否为减益（敌人身上的）
    bool is_expired = false;         // 是否已过期
};

// ============================================================================
// 【PollutedSpirit】污染灵体（敌人）
// ============================================================================
struct PollutedSpirit {
    std::string id;
    std::string name;
    SpiritType type = SpiritType::Common;
    ElementType element = ElementType::Neutral;

    // 位置与移动
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;
    float speed = 60.0f;

    // 污染值
    float max_pollution = 100.0f;
    float current_pollution = 100.0f;
    float pollution_regen_per_second = 0.0f; // 负数=持续被污染

    // 干扰抗性（类似血量，攻击叠加上限）
    float max_health = 100.0f;
    float current_health = 100.0f;

    // 碰撞
    float size_radius = 20.0f;

    // AI
    SpiritAIState ai_state = SpiritAIState::Idle;
    float attack_cooldown = 3.0f;    // 攻击间隔（秒）
    float attack_timer = 0.0f;        // 当前攻击计时
    float attack_range = 80.0f;       // 攻击范围
    float attack_damage = 10.0f;       // 每次攻击的干扰伤害

    // 状态
    bool is_defeated = false;    // 是否已被净化
    bool is_escaped = false;     // 是否暂时退散
    bool is_stunned = false;     // 是否被晕
    float stun_remaining = 0.0f; // 晕眩剩余时间

    // 奖励
    std::string defeat_dialogue;
    std::vector<std::string> reward_item_ids;
    float reward_exp = 10.0f;

    // 视觉
    std::string sprite_id;
    std::string particle_effect_id;
};

// ============================================================================
// 【BattlePartner】战场灵兽伙伴
// ============================================================================
struct BattlePartner {
    std::string spirit_beast_id;  // 关联灵兽数据ID
    std::string name;

    int heart_level = 1;              // 羁绊等级（1-5）
    float purification_rate_mod = 1.0f; // 净化效率修正

    std::vector<std::string> active_skill_ids; // 当前可用技能
    std::vector<float> cooldown_remaining;    // 各技能冷却剩余
    std::vector<float> cooldown_total;        // 各技能总冷却

    bool is_exhausted = false;  // 是否疲劳（战斗后30分钟）
    float aura_range = 100.0f; // 气场范围

    // 位置
    float pos_x = 0.0f;
    float pos_y = 0.0f;
};

// ============================================================================
// 【BattlePlayerState】玩家战斗状态
// ============================================================================
struct BattlePlayerState {
    float max_energy = 100.0f;
    float current_energy = 100.0f;
    float energy_recover_rate = 5.0f; // 每秒回复

    float purification_rate = 1.0f; // 玩家基础净化效率

    // 技能
    std::vector<std::string> unlocked_skill_ids;
    std::vector<float> cooldown_remaining;
    std::vector<float> cooldown_total;

    // 护盾
    float shield_points = 0.0f;
    float shield_duration = 0.0f;

    // Buff列表
    std::vector<Buff> active_buffs;

    // 属性
    float dodge_chance = 0.0f;
    float crit_chance = 0.0f;

    // 状态
    bool is_defeated = false; // 玩家不存在"战败"，永远为false
};

// ============================================================================
// 【BattleAction】战斗行动记录
// ============================================================================
struct BattleAction {
    float timestamp = 0.0f;
    std::string actor_id;
    std::string actor_name;
    ActionType type;
    std::string skill_id;
    std::string skill_name;
    std::string target_id;
    std::string target_name;
    float energy_cost = 0.0f;
    float effect_value = 0.0f;   // 实际效果值（污染削减量/治疗量）
    bool is_crit = false;        // 是否暴击
    bool is_miss = false;         // 是否闪避
};

// ============================================================================
// 【BattleLogEntry】战斗日志条目
// ============================================================================
struct BattleLogEntry {
    float timestamp = 0.0f;
    std::string message;
    bool is_player_action = true;
    bool is_important = false; // 重要事件（净化完成/BOSS出场等）
};

// ============================================================================
// 【BattleZone】战场区域配置
// ============================================================================
struct BattleZone {
    std::string id;
    std::string name;
    std::string background_sprite_id;
    float ambient_pollution_rate = 1.0f; // 该区域基础污染率倍率
    std::vector<std::string> allowed_spirit_ids; // 允许出现的灵体ID列表
    bool is_spirit_realm = false;         // 是否为灵界区域
};

// ============================================================================
// 【BattleResult】战斗结算
// ============================================================================
struct BattleResult {
    bool victory = false;
    int spirits_purified = 0;
    int spirits_total = 0;
    float total_exp_gained = 0.0f;
    std::vector<std::string> items_gained;
    std::vector<std::pair<std::string, int>> partner_favor_gained; // 灵兽ID, 好感增量
    float battle_duration = 0.0f;
};

}  // namespace CloudSeamanor::engine
