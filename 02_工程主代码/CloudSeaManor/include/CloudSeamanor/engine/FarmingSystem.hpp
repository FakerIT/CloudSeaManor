#pragma once

// ============================================================================
// 【FarmingSystem】农业系统
// ============================================================================
// 管理作物种植、生长和收获的完整流程。
//
// 主要职责：
// - 管理地块列表和状态
// - 处理种植、浇水、收获交互
// - 更新作物生长状态
// - 计算农业相关奖励
// - 集成工具系统效果（锄头效率、水壶范围等）
//
// 设计原则：
// - 独立的农业状态管理
// - 支持多种作物类型
// - 与天气、技能和工具系统联动
// ============================================================================

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/ToolSystem.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【FarmingCallbacks】农业系统回调函数
// ============================================================================
struct FarmingCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
    std::function<void(TeaPlot&, bool)> refresh_plot_visual;
    std::function<void(TeaPlot&, const CloudSeamanor::domain::CloudSystem&)> apply_planting_snapshot;
};

// ============================================================================
// 【FarmingResult】农业操作结果
// ============================================================================
struct FarmingResult {
    bool success = false;
    std::string message;
    std::string harvested_item_id;
    int harvested_amount = 0;
    bool skill_level_up = false;
    std::string skill_name;
    int new_level = 0;
    // ========== 新增字段 ==========
    /** 收获时提供的饱食恢复值 */
    int hunger_restore = 0;
    /** 收获时的品质等级 */
    CloudSeamanor::domain::CropQuality quality = CloudSeamanor::domain::CropQuality::Normal;
    /** 是否为灵化变种 */
    bool spirit_mutated = false;
};

// ============================================================================
// 【FarmingSystem】农业系统类
// ============================================================================
class FarmingSystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    FarmingSystem();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(const FarmingCallbacks& callbacks);
    void BuildDefaultPlots();

    // ========================================================================
    // 【地块访问】
    // ========================================================================
    [[nodiscard]] std::vector<TeaPlot>& Plots() { return plots_; }
    [[nodiscard]] const std::vector<TeaPlot>& Plots() const { return plots_; }

    // ========================================================================
    // 【地块交互】
    // ========================================================================
    FarmingResult HandleInteraction(
        int plot_index,
        CloudSeamanor::domain::Inventory& inventory,
        CloudSeamanor::domain::SkillSystem& skills,
        float cloud_density,
        bool spirit_beast_interacted,
        const CloudSeamanor::domain::CloudSystem& cloud_system,
        const CloudSeamanor::domain::CropTable& crop_table,
        const CloudSeamanor::domain::ToolSystem* tool_system = nullptr
    );

    // ========================================================================
    // 【生长更新】
    // ========================================================================
    void UpdateGrowth(float delta_seconds, float cloud_multiplier);
    void ResetDailyState(const std::function<void(TeaPlot&, bool)>& refresh_visual);

    // ========================================================================
    // 【统计】
    // ========================================================================
    [[nodiscard]] int CountReadyPlots() const;
    [[nodiscard]] int CountDryPlots() const;
    [[nodiscard]] int CountGrowingPlots() const;
    [[nodiscard]] int CountSeededPlots() const;

    // ========================================================================
    // 【状态文本】
    // ========================================================================
    [[nodiscard]] std::string GetPlotStatusText(const TeaPlot& plot) const;

private:
    std::vector<TeaPlot> plots_;
    FarmingCallbacks callbacks_;
};

// ============================================================================
// 【CreateDefaultPlotCallbacks】创建默认农业回调
// ============================================================================
FarmingCallbacks CreateDefaultPlotCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void(TeaPlot&, bool)>& refresh_visual
);

} // namespace CloudSeamanor::engine
