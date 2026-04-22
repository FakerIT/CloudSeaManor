#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/FarmingSystem.hpp"

#include "CloudSeamanor/GameAppText.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// 【FarmingSystem】构造函数
// ============================================================================
FarmingSystem::FarmingSystem() {
}

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void FarmingSystem::Initialize(const FarmingCallbacks& callbacks) {
    callbacks_ = callbacks;
}

// ============================================================================
// 【BuildDefaultPlots】构建默认地块
// ============================================================================
void FarmingSystem::BuildDefaultPlots() {
    plots_.clear();
    constexpr float start_x = 392.0f;
    constexpr float start_y = 360.0f;
    constexpr float gap = 64.0f;

    for (int i = 0; i < 3; ++i) {
        TeaPlot plot;
        plot.shape.setSize({52.0f, 52.0f});
        plot.shape.setPosition({start_x + gap * static_cast<float>(i), start_y});
        if (i < 2) {
            plot.crop_name = "云雾茶";
            plot.seed_item_id = "TeaSeed";
            plot.harvest_item_id = "TeaLeaf";
            plot.harvest_amount = 3;
        } else {
            plot.crop_name = "萝卜";
            plot.seed_item_id = "TurnipSeed";
            plot.harvest_item_id = "Turnip";
            plot.harvest_amount = 2;
        }
        // BE-013: 默认生成障碍状态，验证 raw land -> obstacle -> tilled 状态机链路。
        if (i == 0) {
            plot.cleared = false;
            plot.obstacle_type = PlotObstacleType::Stone;
            plot.obstacle_hits_left = 3;
        } else if (i == 1) {
            plot.cleared = false;
            plot.obstacle_type = PlotObstacleType::Stump;
            plot.obstacle_hits_left = 2;
        }
        if (callbacks_.refresh_plot_visual) {
            callbacks_.refresh_plot_visual(plot, false);
        }
        plots_.push_back(plot);
    }
}

// ============================================================================
// 【HandleInteraction】处理地块交互
// ============================================================================
FarmingResult FarmingSystem::HandleInteraction(
    int plot_index,
    CloudSeamanor::domain::Inventory& inventory,
    CloudSeamanor::domain::SkillSystem& skills,
    float cloud_density,
    bool spirit_beast_interacted
) {
    FarmingResult result;

    if (plot_index < 0 || plot_index >= static_cast<int>(plots_.size())) {
        result.message = "无效的地块索引。";
        return result;
    }

    TeaPlot& plot = plots_[static_cast<std::size_t>(plot_index)];

    // 翻土
    if (!plot.tilled) {
        plot.tilled = true;
        result.success = true;
        result.message = plot.crop_name + " 地块已翻土。下一步：播种。";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.4f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 地块已翻土。");
        if (callbacks_.refresh_plot_visual) callbacks_.refresh_plot_visual(plot, true);
        return result;
    }

    // 播种
    if (!plot.seeded) {
        auto remove_result = inventory.TryRemoveItem(plot.seed_item_id, 1);
        if (remove_result) {
            plot.seeded = true;
            plot.growth = 0.0f;
            plot.stage = 1;
            result.success = true;
            result.message = plot.crop_name + " 已播种。给它浇水后就会开始生长。";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.6f);
            if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 已播种。");
        } else {
            result.message = "缺少种子：" + ItemDisplayName(plot.seed_item_id) + "。";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.6f);
            if (callbacks_.log_info) callbacks_.log_info("播种失败：" + remove_result.Error());
        }
        if (callbacks_.refresh_plot_visual) callbacks_.refresh_plot_visual(plot, true);
        return result;
    }

    // 浇水
    if (!plot.watered) {
        plot.watered = true;
        result.success = true;
        result.message = plot.crop_name + " 已浇水。当前云海只会提供正向加成，可以安心等它成长。";
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.0f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 地块已浇水。");
        if (callbacks_.refresh_plot_visual) callbacks_.refresh_plot_visual(plot, true);
        return result;
    }

    // 收获
    if (plot.ready) {
        const float tea_buff = 1.0f + skills.GetBonus(CloudSeamanor::domain::SkillType::SpiritFarm) * 0.1f;
        const float beast_share = spirit_beast_interacted ? 1.2f : 1.0f;

        if (skills.AddExp(CloudSeamanor::domain::SkillType::SpiritFarm, 20.0f, cloud_density, tea_buff, beast_share)) {
            result.skill_level_up = true;
            result.skill_name = skills.GetSkillName(CloudSeamanor::domain::SkillType::SpiritFarm);
            result.new_level = skills.GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
            result.message = "灵农技能提升至 Lv." + std::to_string(result.new_level) + "！加成效果增强。";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.2f);
            if (callbacks_.log_info) callbacks_.log_info("灵农技能升级至 Lv." + std::to_string(result.new_level) + "！");
        }

        auto add_result = inventory.TryAddItem(plot.harvest_item_id, plot.harvest_amount);
        if (!add_result) {
            result.success = false;
            result.message = "收获失败，背包无法添加物品。";
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.8f);
            if (callbacks_.log_info) callbacks_.log_info("收获失败：" + add_result.Error());
            return result;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;

        result.success = true;
        result.harvested_item_id = plot.harvest_item_id;
        result.harvested_amount = plot.harvest_amount;
        std::string harvest_msg = "已收获 " + ItemDisplayName(plot.harvest_item_id) + " x" + std::to_string(plot.harvest_amount) + "。";
        if (callbacks_.push_hint) callbacks_.push_hint(harvest_msg, 2.8f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 已收获。");
        if (callbacks_.refresh_plot_visual) callbacks_.refresh_plot_visual(plot, true);
        return result;
    }

    // 生长中
    result.message = plot.crop_name + " 还在生长中。" + GetPlotStatusText(plot) + "。";
    if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.2f);
    if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 仍在生长中。");
    if (callbacks_.refresh_plot_visual) callbacks_.refresh_plot_visual(plot, true);
    return result;
}

// ============================================================================
// 【UpdateGrowth】更新作物生长
// ============================================================================
void FarmingSystem::UpdateGrowth(float delta_seconds, float cloud_multiplier) {
    for (auto& plot : plots_) {
        if (!plot.seeded || !plot.watered || plot.ready) {
            continue;
        }

        const int previous_stage = plot.stage;
        plot.growth += delta_seconds * cloud_multiplier;

        const int next_stage = std::min(4, static_cast<int>(plot.growth / 20.0f) + 1);
        if (next_stage != plot.stage) {
            plot.stage = next_stage;
            if (plot.stage > previous_stage) {
                if (callbacks_.push_hint) {
                    callbacks_.push_hint(plot.crop_name + " 进入了生长阶段 " + std::to_string(plot.stage) + "。", 2.0f);
                }
            }
        }

        constexpr float kHarvestThreshold = 1.0f;
        if (plot.growth >= plot.growth_time * kHarvestThreshold) {
            plot.ready = true;
            plot.stage = 4;
            if (callbacks_.push_hint) {
                callbacks_.push_hint(plot.crop_name + " 已经可以收获了。", 2.6f);
            }
        }
    }
}

// ============================================================================
// 【ResetDailyState】重置每日状态
// ============================================================================
void FarmingSystem::ResetDailyState(const std::function<void(TeaPlot&, bool)>& refresh_visual) {
    for (auto& plot : plots_) {
        plot.watered = false;
        if (refresh_visual) refresh_visual(plot, false);
    }
}

// ============================================================================
// 【CountReadyPlots】统计可收获地块
// ============================================================================
int FarmingSystem::CountReadyPlots() const {
    int count = 0;
    for (const auto& plot : plots_) {
        if (plot.ready) ++count;
    }
    return count;
}

// ============================================================================
// 【CountDryPlots】统计缺水地块
// ============================================================================
int FarmingSystem::CountDryPlots() const {
    int count = 0;
    for (const auto& plot : plots_) {
        if (!plot.ready && plot.seeded && !plot.watered) ++count;
    }
    return count;
}

// ============================================================================
// 【CountGrowingPlots】统计生长中地块
// ============================================================================
int FarmingSystem::CountGrowingPlots() const {
    int count = 0;
    for (const auto& plot : plots_) {
        if (plot.seeded && plot.watered && !plot.ready) ++count;
    }
    return count;
}

// ============================================================================
// 【CountSeededPlots】统计已播种地块
// ============================================================================
int FarmingSystem::CountSeededPlots() const {
    int count = 0;
    for (const auto& plot : plots_) {
        if (plot.seeded) ++count;
    }
    return count;
}

// ============================================================================
// 【GetPlotStatusText】获取地块状态文本
// ============================================================================
std::string FarmingSystem::GetPlotStatusText(const TeaPlot& plot) const {
    if (!plot.tilled) {
        return "未翻土";
    }
    if (!plot.seeded) {
        return "可播种";
    }
    if (!plot.watered) {
        return "需要浇水";
    }
    if (plot.ready) {
        return "可以收获";
    }
    return "生长阶段 " + std::to_string(plot.stage) + "/4";
}

// ============================================================================
// 【CreateDefaultPlotCallbacks】创建默认农业回调
// ============================================================================
FarmingCallbacks CreateDefaultPlotCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void(TeaPlot&, bool)>& refresh_visual
) {
    FarmingCallbacks callbacks;
    callbacks.push_hint = push_hint;
    callbacks.log_info = log_info;
    callbacks.refresh_plot_visual = refresh_visual;
    return callbacks;
}

} // namespace CloudSeamanor::engine
