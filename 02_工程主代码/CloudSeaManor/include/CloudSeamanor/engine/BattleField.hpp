#pragma once

// ============================================================================
// 【BattleField.hpp】战场逻辑
// ============================================================================
// 管理战斗的核心逻辑：敌人生成、技能释放、污染值计算、胜负判定。
//
// 主要职责：
// - 生成和管理污染灵体
// - 处理技能释放和命中判定
// - 计算污染值削减和净化完成
// - 管理敌人AI行为状态机
// - 计算战斗结果和奖励发放
//
// 与其他系统的关系：
// - 依赖：CloudSystem（天气倍率）、SkillSystem（SpiritGuard技能）
// - 依赖：SpiritBeastSystem（出战灵兽数据）
// - 被依赖：BattleManager（Update驱动）、GameWorldState（存档）
// ============================================================================

#include "CloudSeamanor/engine/BattleEntities.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/SeedDropTable.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <random>
#include <deque>

namespace CloudSeamanor::engine {

// 前向声明
class BattleManager;

// ============================================================================
// 【BattleField】战场逻辑核心
// ============================================================================
class BattleField {
public:
    // 怪物（污染灵体）数据结构，作为 BattleField 的标准输入。
    struct MonsterTableEntry {
        std::string id;
        std::string name;
        std::string zone;
        int star = 1;
        SpiritType type = SpiritType::Common;
        ElementType element = ElementType::Neutral;
        float pollution = 100.0f;
        float health = 100.0f;
        float speed = 60.0f;
        float attack_cooldown = 3.0f;
        float attack_damage = 10.0f;
        float reward_exp = 10.0f;
        std::vector<std::string> reward_item_ids;
        std::string behavior_type;
    };

    struct SkillTableEntry {
        std::string id;
        std::string name;
        float energy_cost = 0.0f;
        float cooldown = 0.0f;
        float base_power = 0.0f;
        ElementType element = ElementType::Neutral;
    };

    // 武器结构：用于玩家战斗前构建属性面板/战斗修正。
    struct WeaponTableEntry {
        std::string id;
        std::string name;
        std::string weapon_type;
        ElementType element = ElementType::Neutral;
        float base_attack = 20.0f;
        float purify_rate_bonus = 0.0f;
        float crit_chance_bonus = 0.0f;
        float crit_multiplier_bonus = 0.0f;
        float energy_recover_bonus = 0.0f;
        float skill_cooldown_scale = 1.0f;
        int quality = 1;
    };

    // 数值总表：用于战斗统一调参，替代硬编码魔法数。
    struct BattleTuningConfig {
        float player_base_crit_chance = 0.05f;
        float player_base_purify_rate = 1.0f;
        float partner_base_skill_power = 25.0f;
        float partner_default_cooldown = 15.0f;
        float area_skill_hit_radius = 160.0f;
    };

    // ========================================================================
    // 【CSV加载】
    // ========================================================================
    bool LoadSpiritTableFromCsv(const std::string& file_path);
    bool LoadSkillTableFromCsv(const std::string& file_path);
    bool LoadZoneTableFromCsv(const std::string& file_path);
    bool LoadWeaponTableFromCsv(const std::string& file_path);
    bool LoadBattleTuningFromCsv(const std::string& file_path);
    bool LoadSeedDropTableFromCsv(const std::string& file_path);

    // ========================================================================
    // 【初始化】
    // ========================================================================

    /** 构造函数 */
    BattleField();

    /**
     * @brief 开始一场战斗
     * @param zone 战场区域配置
     * @param cloud_state 当前云海天气状态
     * @param num_common 生成的普通灵体数量
     * @param num_elite 生成的精英灵体数量
     * @param boss_id BOSS的ID（可选）
     */
    void StartBattle(
        const BattleZone& zone,
        CloudSeamanor::domain::CloudState cloud_state,
        int num_common,
        int num_elite,
        const std::optional<std::string>& boss_id = std::nullopt
    );

    /** 结束战斗并清理 */
    void EndBattle();

    // ========================================================================
    // 【帧更新】
    // ========================================================================

    /**
     * @brief 每帧更新战场逻辑
     * @param delta_seconds 帧间隔
     * @param player_pos 玩家当前位置
     * @return 是否还在战斗中
     */
    bool Update(float delta_seconds, float player_pos_x, float player_pos_y);

    // ========================================================================
    // 【技能系统】
    // ========================================================================

    /**
     * @brief 玩家释放技能
     * @param skill_id 技能ID
     * @param target_id 目标灵体ID（可选，范围技能填nullopt）
     * @param target_pos 目标坐标（范围技能时使用）
     * @return 释放是否成功
     */
    bool PlayerCastSkill(
        const std::string& skill_id,
        const std::optional<std::string>& target_id,
        float target_pos_x,
        float target_pos_y
    );

    /**
     * @brief 灵兽伙伴释放技能
     * @param partner_index 伙伴索引
     * @param delta_seconds 帧间隔（用于AI决策）
     */
    void PartnerUpdate(float delta_seconds, float player_pos_x, float player_pos_y);

    // ========================================================================
    // 【查询】
    // ========================================================================

    [[nodiscard]] bool IsActive() const { return is_active_; }
    [[nodiscard]] bool IsVictory() const { return is_victory_; }
    [[nodiscard]] float ElapsedTime() const { return elapsed_time_; }

    [[nodiscard]] const std::vector<PollutedSpirit>& GetSpirits() const { return spirits_; }
    [[nodiscard]] std::vector<PollutedSpirit>& MutableSpirits() { return spirits_; }

    [[nodiscard]] const BattlePlayerState& GetPlayerState() const { return player_state_; }
    [[nodiscard]] BattlePlayerState& MutablePlayerState() { return player_state_; }

    [[nodiscard]] const std::vector<BattlePartner>& GetPartners() const { return partners_; }
    [[nodiscard]] std::vector<BattlePartner>& MutablePartners() { return partners_; }

    [[nodiscard]] const std::vector<BattleLogEntry>& GetLog() const { return battle_log_; }
    [[nodiscard]] float GetWeatherMultiplier() const { return weather_multiplier_; }

    [[nodiscard]] const BattleResult& GetResult() const { return result_; }
    [[nodiscard]] const std::vector<BattleHitEffect>& GetHitEffects() const { return hit_effects_; }
    [[nodiscard]] const std::unordered_map<std::string, WeaponTableEntry>& GetWeaponTable() const { return weapon_table_; }
    [[nodiscard]] const std::unordered_map<std::string, MonsterTableEntry>& GetMonsterTable() const { return monster_table_; }
    [[nodiscard]] const BattleTuningConfig& GetTuningConfig() const { return tuning_config_; }

    // ========================================================================
    // 【灵兽伙伴管理】
    // ========================================================================

    /** 添加出战灵兽伙伴 */
    void AddPartner(const BattlePartner& partner);

    /** 移除出战灵兽伙伴 */
    void RemovePartner(const std::string& spirit_beast_id);

    // ========================================================================
    // 【战斗日志】
    // ========================================================================

    void AddLog(const std::string& message, bool is_player_action = true, bool important = false);
    void ClearLog();
    bool EquipWeapon(const std::string& weapon_id);
    [[nodiscard]] const std::string& GetEquippedWeaponId() const { return equipped_weapon_id_; }
    void SetQuestSkills(const std::vector<std::string>& quest_skill_ids);

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

    // 污染值计算
    float CalculatePurifyDamage_(
        float base_damage,
        float purify_rate,
        float weather_mult,
        int heart_level,
        ElementType skill_element,
        ElementType target_element,
        bool is_crit
    ) const;

    // 元素克制判定
    [[nodiscard]] float GetElementMultiplier_(ElementType skill, ElementType target) const;

    // 碰撞检测：圆形
    [[nodiscard]] bool CircleCollision_(
        float x1, float y1, float r1,
        float x2, float y2, float r2
    ) const;

    // 命中判定
    void ApplySkillHit_(
        const std::string& skill_id,
        PollutedSpirit& spirit,
        float damage,
        bool is_crit
    );

    // 胜负判定
    void CheckVictoryCondition_();

    // 敌人AI更新
    void UpdateSpiritAI_(PollutedSpirit& spirit, float delta, float player_x, float player_y);

    // 结算奖励
    void CalculateRewards_();
    void RollSeedDrops_();
    void ApplyWeaponStatsToPlayer_();
    bool RollCritical_();
    [[nodiscard]] bool HasQuestSkill_(const std::string& id) const;
    void ResetQuestSkillRuntime_();
    void InitializeImbalanceSegments_(PollutedSpirit& spirit);
    void PushTeaAction_(const std::string& skill_id);
    void TryTriggerTeaCombo_();

    [[nodiscard]] std::string GetWeatherText_() const;

    // ========================================================================
    // 成员变量
    // ========================================================================

    bool is_active_ = false;
    bool is_victory_ = false;
    float elapsed_time_ = 0.0f;
    BattleZone current_zone_;

    std::vector<PollutedSpirit> spirits_;
    std::vector<BattlePartner> partners_;
    BattlePlayerState player_state_;

    CloudSeamanor::domain::CloudState cloud_state_ = CloudSeamanor::domain::CloudState::Clear;
    float weather_multiplier_ = 1.0f;

    std::vector<BattleLogEntry> battle_log_;
    std::vector<BattleAction> battle_history_;
    std::vector<BattleHitEffect> hit_effects_;

    BattleResult result_;

    std::unordered_map<std::string, PollutedSpirit> spirit_table_;
    std::unordered_map<std::string, MonsterTableEntry> monster_table_;
    std::unordered_map<std::string, SkillTableEntry> skill_table_;
    std::unordered_map<std::string, WeaponTableEntry> weapon_table_;
    std::unordered_map<std::string, BattleZone> zone_table_;
    BattleTuningConfig tuning_config_;
    std::string equipped_weapon_id_;
    std::mt19937 random_engine_;
    std::uniform_real_distribution<float> random_unit_{0.0f, 1.0f};
    std::vector<std::string> active_quest_skill_ids_;
    bool quest_first_hit_reduction_available_ = false;
    std::deque<std::string> tea_combo_history_;

    // 种子掉落系统
    CloudSeamanor::domain::SeedDropTable seed_drop_table_;
    std::vector<std::string> rolled_seed_drops_;

    // 净化计数器
    int spirits_purified_count_ = 0;
    int spirits_total_count_ = 0;
};

}  // namespace CloudSeamanor::engine
