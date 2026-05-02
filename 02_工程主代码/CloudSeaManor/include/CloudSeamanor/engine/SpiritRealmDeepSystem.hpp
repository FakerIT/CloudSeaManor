#pragma once

// ============================================================================
// 【SpiritRealmDeepSystem.hpp】灵界深层系统
// ============================================================================
// 灵界深层区域特有的机制：毒云、首领战斗、高难度挑战。
//
// 主要职责：
// - 管理灵界层级的状态和转换
// - 处理有毒云雾伤害
// - 管理首领战斗的触发和奖励
// - 提供进入条件判定
//
// 设计原则：
// - 数据驱动：通过CSV配置各层级参数
// - 与BattleManager集成处理首领战斗
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【SpiritRealmLayerConfig】灵界层级配置
// ============================================================================
struct SpiritRealmLayerConfig {
    std::string id;
    std::string name;
    int min_level = 1;
    int required_pacts = 0;
    float ambient_pollution_rate = 1.0f;
    float toxic_cloud_damage = 0.0f;  // 每秒伤害，0表示无毒云
    float drop_multiplier = 1.0f;
    std::string bgm_id;
    std::string background_sprite;
    std::string unlock_condition;
};

// ============================================================================
// 【BossConfig】首领配置
// ============================================================================
struct BossConfig {
    std::string id;
    std::string name;
    int layer = 2;
    int min_level = 25;
    float max_pollution = 500.0f;
    float health = 300.0f;
    float attack_damage = 25.0f;
    float attack_interval = 2.5f;
    float speed = 50.0f;
    float size_radius = 30.0f;
    std::string element_type;
    std::string sprite_id;
    int phase_count = 3;
    std::vector<std::string> skill_ids;
    float reward_exp = 80.0f;
    std::vector<std::pair<std::string, int>> reward_items;  // item_id, count
    std::string dialogue_on_spawn;
};

// ============================================================================
// 【SpiritRealmDeepSystem】灵界深层系统
// ============================================================================
class SpiritRealmDeepSystem {
public:
    SpiritRealmDeepSystem() = default;

    /**
     * @brief 加载层级配置
     */
    void LoadLayerConfigs(const std::string& csv_path);

    /**
     * @brief 加载首领配置
     */
    void LoadBossConfigs(const std::string& csv_path);

    // ========================================================================
    // 【层级管理】
    // ========================================================================

    /**
     * @brief 获取当前层级配置
     */
    [[nodiscard]] const SpiritRealmLayerConfig* GetLayerConfig(int layer) const;

    /**
     * @brief 检查是否可进入指定层级
     */
    [[nodiscard]] bool CanEnterLayer(int layer, int player_level, int completed_pacts) const;

    /**
     * @brief 获取所有可进入的层级
     */
    [[nodiscard]] std::vector<const SpiritRealmLayerConfig*> GetUnlockedLayers(
        int player_level, int completed_pacts) const;

    // ========================================================================
    // 【毒云伤害】
    // ========================================================================

    /**
     * @brief 获取当前层级每秒毒云伤害
     */
    [[nodiscard]] float GetToxicCloudDamage(int layer) const;

    /**
     * @brief 计算毒云累积伤害
     */
    [[nodiscard]] float CalculateToxicDamage(float damage_per_second, float duration_seconds) const;

    // ========================================================================
    // 【首领管理】
    // ========================================================================

    /**
     * @brief 获取指定层级的首领配置
     */
    [[nodiscard]] const BossConfig* GetBossForLayer(int layer) const;

    /**
     * @brief 获取所有首领
     */
    [[nodiscard]] const std::vector<BossConfig>& GetAllBosses() const { return bosses_; }

    /**
     * @brief 检查是否击败过某首领
     */
    [[nodiscard]] bool IsBossDefeated(const std::string& boss_id) const;

    /**
     * @brief 标记首领为已击败
     */
    void MarkBossDefeated(const std::string& boss_id);

    /**
     * @brief 重置首领击败状态
     */
    void ResetBossStates();

    // ========================================================================
    // 【存档集成】
    // ========================================================================

    /**
     * @brief 获取已击败首领列表（用于存档）
     */
    [[nodiscard]] std::vector<std::string> GetDefeatedBossIds() const;

    /**
     * @brief 从存档恢复首领状态
     */
    void LoadDefeatedBosses(const std::vector<std::string>& defeated_ids);

private:
    std::unordered_map<int, SpiritRealmLayerConfig> layer_configs_;
    std::vector<BossConfig> bosses_;
    std::unordered_map<std::string, bool> defeated_bosses_;
};

}  // namespace CloudSeamanor::engine
