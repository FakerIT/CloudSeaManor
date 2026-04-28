#pragma once

// ============================================================================
// 【ToolSystem】工具升级系统
// ============================================================================
// 管理玩家的锄头、水壶、洒水器升级路径与效果加成。
//
// 主要职责：
// - 定义工具类型和升级阶段（普通 → 铜 → 银 → 金 → 灵金）
// - 管理工具当前等级和经验值
// - 提供工具效果加成查询（范围、效率、消耗减免）
// - 支持数据驱动的工具定义（从配置加载）
//
// 升级设计：
// - 每种工具有 5 级（Lv.1 ~ Lv.5）
// - 使用工具获得经验，达到阈值后自动升级
// - 更高等级 = 更广范围 / 更低消耗 / 更高效率
// - Lv.5 灵金级工具解锁特殊效果
//
// 设计原则：
// - 纯领域层，不依赖渲染或 UI
// - 工具效果加成在所有相关计算中生效（收割、浇水等）
// ============================================================================

#include <string>
#include <unordered_map>

namespace CloudSeamanor::domain {

// ============================================================================
// 【ToolType】工具类型
// ============================================================================
enum class ToolType : std::uint8_t {
    Hoe,         // 锄头（翻土效率）
    WateringCan,  // 水壶（浇水范围/容量）
    Sprinkler,   // 洒水器（自动浇水范围）
    Sickle,      // 镰刀（收割效率/范围）
    Scissors,    // 剪刀（采集效率）
    Axe,         // 斧头（清除障碍效率）
    Pickaxe,     // 镐子（采矿效率）
    Count
};

// ============================================================================
// 【ToolTier】工具品质阶段
// ============================================================================
enum class ToolTier : std::uint8_t {
    Tier1 = 0,  // 普通
    Tier2 = 1,  // 铜
    Tier3 = 2,  // 银
    Tier4 = 3,  // 金
    Tier5 = 4,  // 灵金
    COUNT = 5
};

// ============================================================================
// 【ToolLevel】工具等级
// ============================================================================
struct ToolLevel {
    ToolTier tier = ToolTier::Tier1;
    int level = 1;         // 1~5
    float exp = 0.0f;
    float exp_for_next = 50.0f;  // 升级所需经验
};

// ============================================================================
// 【ToolEffect】工具效果加成
// ============================================================================
struct ToolEffect {
    /** 工具名 */
    std::string name;

    /** 工具名（中文） */
    std::string display_name;

    /** 效果描述 */
    std::string description;

    /** 使用效率倍率（>1.0 更高效） */
    float efficiency_multiplier = 1.0f;

    /** 范围加成倍率（>1.0 范围更大） */
    float range_multiplier = 1.0f;

    /** 消耗减免（0.0~1.0，0.2 = 节省20%体力） */
    float cost_reduction = 0.0f;

    /** 洒水器：每次自动浇水覆盖的地块数 */
    int sprinkler_coverage = 1;

    /** 收集效率倍率（镰刀/剪刀收割时产量加成） */
    float collect_efficiency = 1.0f;

    /** 收集范围倍率（镰刀/剪刀收割范围） */
    float collect_range = 1.0f;
};

// ============================================================================
// 【ToolUpgradeConfig】工具升级配置
// ============================================================================
struct ToolUpgradeConfig {
    float exp_per_use = 10.0f;    // 每次使用获得经验
    float exp_threshold_tier2 = 50.0f;
    float exp_threshold_tier3 = 150.0f;
    float exp_threshold_tier4 = 400.0f;
    float exp_threshold_tier5 = 1000.0f;
};

// ============================================================================
// 【ToolSystem】工具系统
// ============================================================================
class ToolSystem {
public:
    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(const ToolUpgradeConfig& config);
    bool LoadFromFile(const std::string& file_path = "assets/data/tool/tool_data.csv");

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] int GetLevel(ToolType type) const;
    [[nodiscard]] ToolTier GetTier(ToolType type) const;
    [[nodiscard]] float GetExp(ToolType type) const;
    [[nodiscard]] ToolEffect GetEffect(ToolType type) const;

    // ========================================================================
    // 【经验与升级】
    // ========================================================================
    /** 增加工具经验，返回是否升级 */
    bool AddExp(ToolType type, float amount);

    /** 强制设置等级（用于测试或作弊） */
    void SetLevel(ToolType type, int level);

    // ========================================================================
    // 【效果计算】
    // ========================================================================
    /** 获取工具效率倍率 */
    [[nodiscard]] float GetEfficiency(ToolType type) const;

    /** 获取工具范围倍率 */
    [[nodiscard]] float GetRange(ToolType type) const;

    /** 获取工具消耗减免 */
    [[nodiscard]] float GetCostReduction(ToolType type) const;

    /** 获取洒水器覆盖数 */
    [[nodiscard]] int GetSprinklerCoverage(ToolType type) const;

    /** 获取收集效率倍率（镰刀/剪刀等） */
    [[nodiscard]] float GetCollectEfficiency(ToolType type) const;

    /** 获取收集范围倍率 */
    [[nodiscard]] float GetCollectRange(ToolType type) const;

    /** 计算实际体力消耗（含减免） */
    [[nodiscard]] float CalcActualStaminaCost(ToolType type, float base_cost) const;

    // ========================================================================
    // 【工具名】
    // ========================================================================
    [[nodiscard]] const char* ToolTypeName(ToolType type) const;
    [[nodiscard]] const char* ToolTierName(ToolTier tier) const;
    [[nodiscard]] std::string FullToolName(ToolType type) const;

private:
    // 内部：更新升级阈值
    void RecalculateExpThreshold_(ToolType type);
    // 内部：检查是否升级
    bool CheckLevelUp_(ToolType type);

    std::unordered_map<ToolType, ToolLevel> tool_levels_;
    std::unordered_map<ToolType, ToolEffect> tool_effects_;
    ToolUpgradeConfig config_;
};

// ============================================================================
// 【GetToolEffectTable】获取工具效果表
// ============================================================================
/**
 * @brief 获取指定工具类型和等级的效果。
 * 表驱动，无需动态计算。
 */
const ToolEffect& GetToolEffectTable(ToolType type, int level);

}  // namespace CloudSeamanor::domain
