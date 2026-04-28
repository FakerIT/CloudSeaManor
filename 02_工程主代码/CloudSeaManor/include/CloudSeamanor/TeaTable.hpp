#pragma once

// ============================================================================
// 【TeaTable】灵茶数据驱动层
// ============================================================================
// 负责从 CSV 加载灵茶定义表，提供灵茶信息查询和Buff效果关联。
//
// 主要职责：
// - 从 `assets/data/TeaTable.csv` 加载灵茶定义
// - 按灵茶 ID 查询灵茶定义
// - 关联灵茶与Buff效果
// - 提供灵茶分类查询（按季节、按层级、按茶类）
//
// 设计原则：
// - 所有灵茶参数由数据驱动
// - 灵茶品质受云海状态影响
// - 使用单例模式持有灵茶表
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【TeaDefinition】灵茶定义（单行 CSV 数据）
// ============================================================================
struct TeaDefinition {
    std::string id;                    // 灵茶唯一标识
    std::string name;                  // 中文名称
    int tier = 1;                     // 品质层级 (1=入门, 2=中级, 3=高级, 4=传说)
    std::string season;               // 适种季节 (spring/summer/autumn/winter/all)
    int growth_days = 1;              // 生长周期（游戏内天数）
    int base_yield = 1;               // 基础产量
    int base_price = 0;               // 基础售价
    std::string buff_effect_id;       // Buff效果ID
    std::string unlock_condition;      // 解锁条件 (default/favor_N/contract_xxx/main_chX)
    bool is_legendary = false;        // 是否传说级
    // ========== 新增字段 ==========
    std::string description;          // 简短描述
    std::string lore;                 // 背景传说
    std::string color_hex;            // 茶汤颜色（十六进制）
    std::string tea_type;             // 茶类 (green/white/oolong/black/red/purple/gold)
    float fertilizer_multiplier = 1.0f; // 肥料效果倍率
    std::string cloud_min;            // 最低云海要求 (clear/mist/dense/tide)
    std::string harvest_time;         // 最佳采摘时间 (dawn/day/dusk/night/any)
    std::vector<std::string> season_tags; // 季节标签列表
    std::vector<std::string> tags;    // 标签列表
};

// ============================================================================
// 【TeaTable】灵茶数据表管理器
// ============================================================================
class TeaTable {
public:
    // ========================================================================
    // 【LoadFromFile】从 CSV 文件加载灵茶表
    // ========================================================================
    // @param file_path CSV 文件路径（相对于游戏工作目录）
    // @return true 表示加载成功
    // @note 首次调用时自动加载；后续调用返回缓存结果
    bool LoadFromFile(const std::string& file_path);

    // ========================================================================
    // 【Get】按 ID 获取灵茶定义
    // ========================================================================
    // @param id 灵茶标识
    // @return 灵茶定义指针，未找到返回 nullptr
    [[nodiscard]] const TeaDefinition* Get(const std::string& id) const;

    // ========================================================================
    // 【Exists】检查灵茶是否存在
    // ========================================================================
    [[nodiscard]] bool Exists(const std::string& id) const;

    // ========================================================================
    // 【All】获取所有灵茶定义
    // ========================================================================
    [[nodiscard]] const std::vector<TeaDefinition>& All() const noexcept { return teas_; }

    // ========================================================================
    // 【GetBySeason】按季节获取灵茶列表
    // ========================================================================
    // @param season 季节 (spring/summer/autumn/winter)
    // @return 该季节可种的灵茶列表
    [[nodiscard]] std::vector<const TeaDefinition*> GetBySeason(const std::string& season) const;

    // ========================================================================
    // 【GetByTier】按层级获取灵茶列表
    // ========================================================================
    // @param tier 品质层级 (1-4)
    // @return 该层级的灵茶列表
    [[nodiscard]] std::vector<const TeaDefinition*> GetByTier(int tier) const;

    // ========================================================================
    // 【GetByTeaType】按茶类获取灵茶列表
    // ========================================================================
    // @param tea_type 茶类 (green/white/oolong/black/red/purple/gold)
    // @return 该茶类的灵茶列表
    [[nodiscard]] std::vector<const TeaDefinition*> GetByTeaType(const std::string& tea_type) const;

    // ========================================================================
    // 【GetByTag】按标签获取灵茶列表
    // ========================================================================
    // @param tag 标签名称
    // @return 匹配标签的灵茶列表
    [[nodiscard]] std::vector<const TeaDefinition*> GetByTag(const std::string& tag) const;

    // ========================================================================
    // 【GetLegendaryTeas】获取所有传说级灵茶
    // ========================================================================
    [[nodiscard]] std::vector<const TeaDefinition*> GetLegendaryTeas() const;

    // ========================================================================
    // 【GetAvailableTeas】获取当前可用的灵茶（基于解锁条件）
    // ========================================================================
    // @param favor_levels 当前好感度等级映射
    // @param contract_completed 已完成的契约ID列表
    // @param main_chapter 当前主线章节
    // @return 当前可用的灵茶列表
    [[nodiscard]] std::vector<const TeaDefinition*> GetAvailableTeas(
        const std::unordered_map<std::string, int>& favor_levels = {},
        const std::vector<std::string>& contract_completed = {},
        int main_chapter = 1) const;

    // ========================================================================
    // 【IsUnlocked】检查灵茶是否解锁
    // ========================================================================
    // @param condition 解锁条件字符串
    // @param favor_levels 当前好感度等级映射
    // @param contract_completed 已完成的契约ID列表
    // @param main_chapter 当前主线章节
    // @return 是否解锁
    [[nodiscard]] bool IsUnlocked(
        const std::string& condition,
        const std::unordered_map<std::string, int>& favor_levels,
        const std::vector<std::string>& contract_completed,
        int main_chapter) const;

    // ========================================================================
    // 【CalculateSellPrice】计算灵茶售价（考虑品质）
    // ========================================================================
    // @param tea_id 灵茶ID
    // @param quality_tier 品质等级 (0=普通, 1=优质, 2=稀有, 3=灵品, 4=圣品)
    // @return 计算后的售价
    [[nodiscard]] int CalculateSellPrice(const std::string& tea_id, int quality_tier = 0) const;

    // ========================================================================
    // 【CalculateHarvestTime】计算灵茶收获时间（考虑云海）
    // ========================================================================
    // @param tea_id 灵茶ID
    // @param cloud_state 当前云海状态
    // @return 实际需要的游戏天数
    [[nodiscard]] int CalculateHarvestTime(const std::string& tea_id, const std::string& cloud_state) const;

    // ========================================================================
    // 【TierToText】层级转中文文本
    // ========================================================================
    [[nodiscard]] static const char* TierToText(int tier);

    // ========================================================================
    // 【TeaTypeToText】茶类转中文文本
    // ========================================================================
    [[nodiscard]] static const char* TeaTypeToText(const std::string& tea_type);

    // ========================================================================
    // 【SeasonToText】季节转中文文本
    // ========================================================================
    [[nodiscard]] static const char* SeasonToText(const std::string& season);

    // ========================================================================
    // 【HarvestTimeToText】采摘时间转中文文本
    // ========================================================================
    [[nodiscard]] static const char* HarvestTimeToText(const std::string& harvest_time);

private:
    // ========================================================================
    // 【ParseSeasons_】解析季节字符串
    // ========================================================================
    [[nodiscard]] static std::vector<std::string> ParseSeasons_(const std::string& raw);

    // ========================================================================
    // 【ParseTags_】解析标签字符串
    // ========================================================================
    [[nodiscard]] static std::vector<std::string> ParseTags_(const std::string& raw);

    std::vector<TeaDefinition> teas_;
    std::unordered_map<std::string, std::size_t> index_;
};

// ============================================================================
// 【GetGlobalTeaTable】获取全局灵茶表单例
// ============================================================================
[[nodiscard]] TeaTable& GetGlobalTeaTable();

} // namespace CloudSeamanor::domain
