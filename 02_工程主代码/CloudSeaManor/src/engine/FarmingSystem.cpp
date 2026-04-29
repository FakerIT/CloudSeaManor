#include "CloudSeamanor/engine/FarmingSystem.hpp"

#include "CloudSeamanor/app/GameAppText.hpp"

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
        plot.size = {52.0f, 52.0f};
        plot.position = {start_x + gap * static_cast<float>(i), start_y};
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
    const float,
    const bool spirit_beast_interacted,
    const CloudSeamanor::domain::CloudSystem& cloud_system,
    const CloudSeamanor::domain::CropTable& crop_table,
    const CloudSeamanor::domain::ToolSystem* tool_system
) {
    FarmingResult result;

    if (plot_index < 0 || plot_index >= static_cast<int>(plots_.size())) {
        result.message = "无效的地块索引。";
        return result;
    }

    TeaPlot& plot = plots_[static_cast<std::size_t>(plot_index)];

    // ── 工具效果倍率（从 ToolSystem 获取）───────────────────────────────
    float hoe_efficiency = 1.0f;
    float can_range = 1.0f;
    float sickle_efficiency = 1.0f;
    if (tool_system) {
        hoe_efficiency = tool_system->GetEfficiency(CloudSeamanor::domain::ToolType::Hoe);
        can_range = tool_system->GetRange(CloudSeamanor::domain::ToolType::WateringCan);
        sickle_efficiency = tool_system->GetCollectEfficiency(CloudSeamanor::domain::ToolType::Sickle);
    }

    // 翻土（锄头效率加成）
    if (!plot.tilled) {
        plot.tilled = true;
        result.success = true;
        const std::string hoe_hint = (hoe_efficiency > 1.0f)
            ? "（锄头加成 +" + std::to_string(static_cast<int>((hoe_efficiency - 1.0f) * 100)) + "%）"
            : "";
        result.message = plot.crop_name + " 地块已翻土。下一步：播种。" + hoe_hint;
        if (callbacks_.push_hint) callbacks_.push_hint(result.message, 2.4f);
        if (callbacks_.log_info) callbacks_.log_info(plot.crop_name + " 地块已翻土。" + hoe_hint);
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
            // 记录品质快照（播种时的云海密度和灵气值）
            if (callbacks_.apply_planting_snapshot) {
                callbacks_.apply_planting_snapshot(plot, cloud_system);
            }
            // 获取作物定义的饱食恢复值
            if (const auto* def = crop_table.Get(plot.crop_id)) {
                plot.hunger_restore = def->hunger_restore;
            }
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
        const float cloud_density = cloud_system.CurrentSpiritDensity();
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

        // 计算最终品质（快照 + 累积加成）
        const auto* def = crop_table.Get(plot.crop_id);
        const float harvest_multiplier = def
            ? CloudSeamanor::domain::CropTable::QualityHarvestMultiplier(plot.quality)
            : 1.0f;

        // 镰刀收割效率加成
        int final_harvest = static_cast<int>(plot.harvest_amount * harvest_multiplier * sickle_efficiency);
        if (plot.spirit_mutated) {
            final_harvest = static_cast<int>(final_harvest * 2.0f);
        }
        final_harvest = std::max(1, final_harvest);

        // 确定收获物品ID（灵化变种加前缀）
        std::string harvest_item_id = plot.harvest_item_id;
        if (plot.spirit_mutated) {
            harvest_item_id = "spirit_" + harvest_item_id;
        }

        auto add_result = inventory.TryAddItem(harvest_item_id, final_harvest);
        if (!add_result) {
            result.success = false;
            // QA-003 改进：提供更明确的失败原因
            std::string fail_reason = add_result.Error();
            if (inventory.IsFull() && inventory.CountOf(harvest_item_id) == 0) {
                result.message = "收获失败，背包已满！请清理背包后重新收获。";
            } else if (fail_reason.find("叠加已达上限") != std::string::npos) {
                result.message = "收获的 [" + ItemDisplayName(harvest_item_id) + "] 叠加已达上限(" +
                    std::to_string(inventory.MaxStackSize()) + ")，部分物品可能丢失。";
            } else {
                result.message = "收获失败：" + fail_reason;
            }
            if (callbacks_.push_hint) callbacks_.push_hint(result.message, 3.5f);
            if (callbacks_.log_info) callbacks_.log_info("收获失败：" + fail_reason);
            return result;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;
        plot.spirit_mutated = false;

        result.success = true;
        result.harvested_item_id = harvest_item_id;
        result.harvested_amount = final_harvest;
        result.quality = plot.quality;
        result.spirit_mutated = plot.spirit_mutated;
        // 饱食恢复（灵化变种 1.5x）
        int hunger = plot.hunger_restore;
        if (plot.spirit_mutated && hunger > 0) {
            hunger = static_cast<int>(hunger * 1.5f);
        }
        result.hunger_restore = hunger;

        // 镰刀加成提示
        std::string sickle_hint = (sickle_efficiency > 1.0f)
            ? "（镰刀加成 +" + std::to_string(static_cast<int>((sickle_efficiency - 1.0f) * 100)) + "%）"
            : "";

        std::string quality_prefix = "";
        if (def) {
            quality_prefix = CloudSeamanor::domain::CropTable::QualityToPrefixText(plot.quality);
        }
        std::string harvest_msg = quality_prefix + "已收获 " + ItemDisplayName(harvest_item_id)
            + " x" + std::to_string(final_harvest) + sickle_hint;
        if (plot.spirit_mutated) {
            harvest_msg += "【灵化变种】";
        }
        harvest_msg += "。";
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
    // Growth updates are unified in CropGrowthSystem::Update.
    // Keep this method as a compatibility no-op to avoid dual growth paths.
    (void)delta_seconds;
    (void)cloud_multiplier;
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
    const std::function<void(TeaPlot&, bool)>& refresh_visual,
    const std::function<void(TeaPlot&, const CloudSeamanor::domain::CloudSystem&)>& apply_snapshot
) {
    FarmingCallbacks callbacks;
    callbacks.push_hint = push_hint;
    callbacks.log_info = log_info;
    callbacks.refresh_plot_visual = refresh_visual;
    callbacks.apply_planting_snapshot = apply_snapshot;
    return callbacks;
}

} // namespace CloudSeamanor::engine
