#pragma once

// ============================================================================
// 【CropData】作物数据驱动层
// ============================================================================
// 负责从 CSV 加载作物定义表，提供作物信息查询和品质计算。
//
// 主要职责：
// - 从 `assets/data/CropTable.csv` 加载作物定义
// - 按作物 ID 查询作物定义
// - 根据云海状态计算作物品质等级
// - 提供品质等级到中文名称的映射
//
// 设计原则：
// - 所有作物参数由数据驱动，无需硬编码
// - 作物品质由云海状态决定（大潮 > 浓云 > 薄雾 > 晴）
// - 使用单例模式持有作物表，确保全局只加载一次
// ============================================================================

#include "CloudSeamanor/CloudSystem.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【CropQuality】作物品质等级（5级）
// ============================================================================
enum class CropQuality {
    Normal = 0,  // 普通（晴/薄雾基础产出）
    Fine     = 1,  // 优质（浓云稳定产出）
    Rare     = 2,  // 稀有（大潮稳定产出）
    Spirit   = 3,  // 灵品（大潮 + 灵品区）
    Holy     = 4,  // 圣品（特殊条件触发：完美大潮 + 全服首次/周年庆）
};

// ============================================================================
// 【CropDefinition】作物定义（单行 CSV 数据）
// ============================================================================
struct CropDefinition {
    std::string id;            // 作物唯一标识
    std::string name;           // 展示名称
    std::string seed_item_id;  // 种子物品 ID
    std::string harvest_item_id; // 收获产物 ID
    float growth_time = 80.0f; // 总生长时间（秒）
    int stages = 4;            // 生长阶段数
    int base_harvest = 2;       // 基础收获数量
    int stamina_cost = 6;      // 每次操作体力消耗
    std::vector<std::string> tags; // 标签（core/rare/premium/legendary/spirit_tea）
    // ========== 新增字段 ==========
    int hunger_restore = 0;              // 饱食恢复值
    std::string special_effect_id;       // 特殊效果ID（可选）
    std::string season_tag = "spring";   // 适种季节（spring/summer/autumn/winter/all）
    float fertilizer_multiplier = 1.0f;  // 肥料效果倍率（灵茶通常更高）
    bool is_spirit_tea = false;         // 是否为灵茶
    std::string cloud_min_requirement; // 最低云海需求（clear/mist/dense/tide）
    std::string buff_effect_id;         // 饮用时的Buff效果ID
    std::vector<std::string> love_npc_ids; // 最爱该作物的角色ID列表
    std::string unlock_condition;         // 解锁条件
    bool is_legendary = false;          // 是否传说级
    // =============================
};

// ============================================================================
// 【CropTable】作物数据表管理器
// ============================================================================
class CropTable {
public:
    // ========================================================================
    // 【LoadFromFile】从 CSV 文件加载作物表
    // ========================================================================
    // @param file_path CSV 文件路径（相对于游戏工作目录）
    // @return true 表示加载成功
    // @note 首次调用时自动加载；后续调用返回缓存结果
    bool LoadFromFile(const std::string& file_path);

    // ========================================================================
    // 【Get】按 ID 获取作物定义
    // ========================================================================
    // @param id 作物标识
    // @return 作物定义指针，未找到返回 nullptr
    [[nodiscard]] const CropDefinition* Get(const std::string& id) const;

    // ========================================================================
    // 【Exists】检查作物是否存在
    // ========================================================================
    [[nodiscard]] bool Exists(const std::string& id) const;

    // ========================================================================
    // 【AllCrops】获取所有作物定义
    // ========================================================================
    [[nodiscard]] const std::vector<CropDefinition>& AllCrops() const noexcept { return crops_; }

    // ========================================================================
    // 【GetByTag】按标签获取作物列表
    // ========================================================================
    // @param tag 标签名称（如 "core"、"rare"）
    // @return 匹配标签的作物列表
    [[nodiscard]] std::vector<const CropDefinition*> GetByTag(const std::string& tag) const;

    // ========================================================================
    // 【CalculateQuality】根据云海状态计算作物品质
    // ========================================================================
    // 品质计算规则：
    //   Clear      → Normal（普通）
    //   Mist       → Normal 或 Fine（薄雾有低概率出优质）
    //   DenseCloud → Fine（浓云稳定出优质）
    //   Tide       → Rare（必定出稀有；SpiritTier = true 时出灵品）
    // @param state 当前云海状态
    // @param is_spirit_tier 是否处于灵品区（山庄满级等条件）
    // @return 对应的作物品质等级
    [[nodiscard]] static CropQuality CalculateQuality(
        CloudState state,
        bool is_spirit_tier = false);

    // ========================================================================
    // 【CalculateFinalQuality】基于快照计算最终品质
    // ========================================================================
    // 综合播种快照、累积云海天数、灵气值、地块加成计算最终品质。
    [[nodiscard]] static CropQuality CalculateFinalQuality(
        CloudState current_state,
        float cloud_density_at_planting,
        int aura_at_planting,
        int dense_cloud_days,
        int tide_days,
        bool tea_soul_nearby,
        const std::string& fertilizer_type,
        bool is_legendary = false);

    // ========================================================================
    // 【QualityToText】品质等级转中文文本
    // ========================================================================
    [[nodiscard]] static const char* QualityToText(CropQuality q);

    // ========================================================================
    // 【QualityToPrefixText】品质等级转带前缀的文本（用于 HUD 提示）
    // ========================================================================
    [[nodiscard]] static std::string QualityToPrefixText(CropQuality q);

    // ========================================================================
    // 【QualityHarvestMultiplier】品质对应的产量倍率
    // ========================================================================
    //   Normal → 1.0x
    //   Fine   → 1.3x
    //   Rare   → 1.8x
    //   Spirit → 2.5x
    [[nodiscard]] static float QualityHarvestMultiplier(CropQuality q);

    // ========================================================================
    // 【StageGrowthThreshold】某阶段的生长阈值（0~1 范围）
    // ========================================================================
    [[nodiscard]] static float StageGrowthThreshold(int stage, int total_stages);

private:
    // ========================================================================
    // 【ParseTags_】解析逗号分隔的标签字符串
    // ========================================================================
    [[nodiscard]] static std::vector<std::string> ParseTags_(const std::string& raw);

    std::vector<CropDefinition> crops_;
    std::unordered_map<std::string, std::size_t> index_;
};

// ============================================================================
// 【GetGlobalCropTable】获取全局作物表单例
// ============================================================================
[[nodiscard]] CropTable& GetGlobalCropTable();

} // namespace CloudSeamanor::domain
