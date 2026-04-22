#pragma once

// ============================================================================
// 【RecipeData】工坊配方数据驱动层
// ============================================================================
// 负责从 CSV 加载工坊配方，提供配方查询和机器绑定。
//
// 主要职责：
// - 从 `assets/data/RecipeTable.csv` 加载配方定义
// - 按配方 ID 查询
// - 按机器 ID 筛选可用配方
// - 提供配方文本构建（用于 HUD 显示）
//
// 设计原则：
// - 所有配方参数由数据驱动，无需硬编码
// - 使用单例模式持有配方表，确保全局只加载一次
// - 配方可按标签分类（core/rare/premium/legendary）
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【RecipeDefinition】配方定义（单行 CSV 数据）
// ============================================================================
struct RecipeDefinition {
    std::string id;              // 配方唯一标识
    std::string name;            // 展示名称
    std::string machine_id;      // 绑定机器 ID
    std::string input_item;       // 输入物品 ID
    int input_count = 1;         // 输入数量
    std::string output_item;     // 输出物品 ID
    int output_count = 1;       // 输出数量
    int process_time = 60;       // 加工时间（秒）
    float base_success_rate = 0.85f; // 基础成功率
    std::vector<std::string> tags;  // 标签（core/rare/premium/legendary）
};

// ============================================================================
// 【RecipeTable】配方数据表管理器
// ============================================================================
class RecipeTable {
public:
    // ========================================================================
    // 【LoadFromFile】从 CSV 文件加载配方表
    // ========================================================================
    // @param file_path CSV 文件路径（相对于游戏工作目录）
    // @return true 表示加载成功
    // @note 首次调用时自动加载；后续调用返回缓存结果
    bool LoadFromFile(const std::string& file_path);

    // ========================================================================
    // 【Get】按 ID 获取配方定义
    // ========================================================================
    [[nodiscard]] const RecipeDefinition* Get(const std::string& id) const;

    // ========================================================================
    // 【GetByMachine】获取指定机器的所有可用配方
    // ========================================================================
    [[nodiscard]] std::vector<const RecipeDefinition*> GetByMachine(
        const std::string& machine_id) const;

    // ========================================================================
    // 【GetByTag】获取指定标签的所有配方
    // ========================================================================
    [[nodiscard]] std::vector<const RecipeDefinition*> GetByTag(
        const std::string& tag) const;

    // ========================================================================
    // 【Exists】检查配方是否存在
    // ========================================================================
    [[nodiscard]] bool Exists(const std::string& id) const;

    // ========================================================================
    // 【AllRecipes】获取所有配方
    // ========================================================================
    [[nodiscard]] const std::vector<RecipeDefinition>& AllRecipes() const noexcept {
        return recipes_;
    }

    // ========================================================================
    // 【AvailableRecipes】获取指定库存可制作的配方
    // ========================================================================
    // @param machine_id 机器 ID
    // @param item_counts 物品库存计数表（item_id -> count）
    // @return 可用配方列表（输入物品数量足够的配方）
    [[nodiscard]] std::vector<const RecipeDefinition*> AvailableRecipes(
        const std::string& machine_id,
        const std::unordered_map<std::string, int>& item_counts) const;

    // ========================================================================
    // 【RecipeDisplayText】构建配方 HUD 显示文本
    // ========================================================================
    [[nodiscard]] static std::string RecipeDisplayText(const RecipeDefinition& recipe);

private:
    // ========================================================================
    // 【ParseTags_】解析分号分隔的标签字符串
    // ========================================================================
    [[nodiscard]] static std::vector<std::string> ParseTags_(const std::string& raw);

    std::vector<RecipeDefinition> recipes_;
    std::unordered_map<std::string, std::size_t> index_;
    std::unordered_multimap<std::string, std::size_t> machine_index_;
};

// ============================================================================
// 【GetGlobalRecipeTable】获取全局配方表单例
// ============================================================================
[[nodiscard]] RecipeTable& GetGlobalRecipeTable();

} // namespace CloudSeamanor::domain
