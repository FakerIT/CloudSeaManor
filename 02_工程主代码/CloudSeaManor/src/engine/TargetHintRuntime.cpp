#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/TargetHintRuntime.hpp"

#include "CloudSeamanor/CropData.hpp"
#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

std::string BuildCurrentTargetText(const TargetHintContext& context) {
    if (context.highlighted_plot_index >= 0) {
        const auto& plot = context.tea_plots[static_cast<std::size_t>(context.highlighted_plot_index)];
        if (!plot.tilled) return plot.crop_name + "：翻土 [E]";
        if (!plot.seeded) {
            const int seed_count = context.inventory.CountOf(plot.seed_item_id);
            if (seed_count > 0) {
                return plot.crop_name + "：种下 " + ItemDisplayName(plot.seed_item_id) + " [E]（持有 x" + std::to_string(seed_count) + "）";
            }
            return plot.crop_name + "：缺少 " + ItemDisplayName(plot.seed_item_id) + "，暂时无法播种";
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
    if (target.Type() == CloudSeamanor::domain::InteractableType::Storage && !context.main_house_repair.completed) {
        const int wood_have = context.inventory.CountOf("Wood");
        const int turnip_have = context.inventory.CountOf("Turnip");
        const bool can_repair = wood_have >= context.main_house_repair.wood_cost &&
                                turnip_have >= context.main_house_repair.turnip_cost;
        if (can_repair) {
            return "主屋：材料齐全，可修缮 [E]（木材 " + std::to_string(wood_have) + "/" + std::to_string(context.main_house_repair.wood_cost) +
                   "，萝卜 " + std::to_string(turnip_have) + "/" + std::to_string(context.main_house_repair.turnip_cost) + "）";
        }
        return "主屋：修缮材料不足（木材 " + std::to_string(wood_have) + "/" + std::to_string(context.main_house_repair.wood_cost) +
               "，萝卜 " + std::to_string(turnip_have) + "/" + std::to_string(context.main_house_repair.turnip_cost) + "）";
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
