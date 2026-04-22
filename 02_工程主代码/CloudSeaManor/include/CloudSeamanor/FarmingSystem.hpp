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
//
// 设计原则：
// - 独立的农业状态管理
// - 支持多种作物类型
// - 与天气和技能系统联动
// ============================================================================

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/SkillSystem.hpp"

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
        bool spirit_beast_interacted
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
