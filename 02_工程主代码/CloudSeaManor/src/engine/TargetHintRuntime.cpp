#include "CloudSeamanor/TargetHintRuntime.hpp"

#include "CloudSeamanor/CropData.hpp"
#include "CloudSeamanor/GameAppFarming.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

std::string BuildCurrentTargetText(const TargetHintContext& context) {
    if (context.highlighted_plot_index >= 0) {
        const auto& plot = context.tea_plots[static_cast<std::size_t>(context.highlighted_plot_index)];
        const std::string layer_text =
            (plot.layer == TeaPlotLayer::TeaGardenExclusive) ? "茶园专属" : "普通农田";
        if (!plot.tilled) return plot.crop_name + "（" + layer_text + "）：翻土 [E]";
        if (!plot.seeded) {
            const int seed_count = context.inventory.CountOf(plot.seed_item_id);
            if (seed_count > 0) {
                return plot.crop_name + "（" + layer_text + "）：种下 "
                    + ItemDisplayName(plot.seed_item_id) + " [E]（持有 x"
                    + std::to_string(seed_count) + "）";
            }
            return plot.crop_name + "（" + layer_text + "）：缺少 "
                + ItemDisplayName(plot.seed_item_id) + "，暂时无法播种";
        }
        if (!plot.watered) return plot.crop_name + "：浇水 [E]";
        if (plot.ready) {
            const std::string quality_prefix =
                domain::CropTable::QualityToPrefixText(plot.quality);
            return plot.crop_name + "：" + quality_prefix + "收获 [E]";
        }
        // 数据驱动：使用 plot.growth_time 而非硬编码
        const int growth_percent =
            std::clamp(static_cast<int>(plot.growth / plot.growth_time * 100.0f), 0, 100);
        return plot.crop_name + "：" + PlotStatusText(plot) + "（成长 " +
               std::to_string(growth_percent) + "%）";
    }

    if (context.highlighted_npc_index >= 0) {
        const auto& npc = context.npcs[static_cast<std::size_t>(context.highlighted_npc_index)];
        return npc.display_name + "：对话 [E] / 赠送茶包 [G]";
    }

    if (context.spirit_beast_highlighted) {
        return std::string("灵兽：结缘 [E] | 协助 ") + (context.spirit_beast_watered_today ? "已使用" : "可使用");
    }

    if (context.highlighted_index < 0) {
        return "在山庄中探索，靠近作物、设施、灵兽或 NPC。";
    }
    const auto& target = context.interactables[static_cast<std::size_t>(context.highlighted_index)];
    if (target.Label() == "Tea Bush") {
        const auto bush_it = std::find_if(
            context.tea_bushes.begin(),
            context.tea_bushes.end(),
            [&](const CloudSeamanor::domain::TeaBush& bush) {
                return bush.id == target.EnemyId();
            });
        if (bush_it == context.tea_bushes.end()) {
            return "茶灌木：靠近采摘 [E]";
        }
        if (bush_it->CanHarvest(context.current_day)) {
            return (bush_it->name.empty() ? "茶灌木" : bush_it->name) + "：可采摘 [E]";
        }
        const int remain_days = bush_it->DaysUntilHarvest(context.current_day);
        const int normalized_hour = std::clamp(context.current_hour, 0, 23);
        const int remain_hours_total =
            std::max(1, (std::max(0, remain_days - 1) * 24) + (24 - normalized_hour));
        const int x_days = remain_hours_total / 24;
        const int y_hours = remain_hours_total % 24;
        return (bush_it->name.empty() ? "茶灌木" : bush_it->name)
            + "：还需 " + std::to_string(x_days) + "天" + std::to_string(y_hours) + "小时 [E]";
    }
    if (target.Type() == CloudSeamanor::domain::InteractableType::Storage) {
        if (context.main_house_repair.level >= kMainHouseMaxLevel) {
            return "主屋：已达最高级（大宅） [E]";
        }
        const auto cost = QueryMainHouseUpgradeCost(context.main_house_repair.level);
        const int wood_have = context.inventory.CountOf("Wood");
        const int turnip_have = context.inventory.CountOf("Turnip");
        const int gold_have = context.player_gold;
        const bool can_upgrade = (wood_have >= cost.wood_cost)
            && (turnip_have >= cost.turnip_cost)
            && (gold_have >= cost.gold_cost);
        if (can_upgrade) {
            return "主屋：材料齐全，可升级到 Lv." + std::to_string(cost.next_level)
                + " [E]（木材 " + std::to_string(wood_have) + "/" + std::to_string(cost.wood_cost)
                + "，萝卜 " + std::to_string(turnip_have) + "/" + std::to_string(cost.turnip_cost)
                + "，金币 " + std::to_string(gold_have) + "/" + std::to_string(cost.gold_cost) + "）";
        }
        return "主屋：升级 Lv." + std::to_string(cost.next_level) + " 材料不足（木材 "
            + std::to_string(wood_have) + "/" + std::to_string(cost.wood_cost)
            + "，萝卜 " + std::to_string(turnip_have) + "/" + std::to_string(cost.turnip_cost)
            + "，金币 " + std::to_string(gold_have) + "/" + std::to_string(cost.gold_cost) + "）";
    }
    if (target.Type() == CloudSeamanor::domain::InteractableType::Workstation) {
        if (const auto* machine = context.workshop.GetMachine("tea_machine")) {
            if (machine->is_processing) {
                return "制茶机：加工中 " + std::to_string(static_cast<int>(machine->progress)) + "%...";
            }
            if (context.tea_machine.queued_output > 0) {
                return "制茶机：加工完成！按 E 领取（待领取 x" +
                    std::to_string(context.tea_machine.queued_output) + "）";
            }
        } else if (context.tea_machine.running) {
            return "制茶机：正在加工茶包";
        } else if (context.tea_machine.queued_output > 0) {
            return "制茶机：加工完成！按 E 领取（待领取 x" +
                std::to_string(context.tea_machine.queued_output) + "）";
        }
        const int tea_leaf_count = context.inventory.CountOf("TeaLeaf");
        if (tea_leaf_count >= 2) {
            return "制茶机：按 E 开始加工茶包（消耗 茶叶 x2，持有 x" + std::to_string(tea_leaf_count) + "）";
        }
        return "制茶机：没有原料可加工（茶叶 " + std::to_string(tea_leaf_count) + "/2）";
    }
    return target.TypeText() + "：" + target.Label() + " [E]";
}

} // namespace CloudSeamanor::engine
