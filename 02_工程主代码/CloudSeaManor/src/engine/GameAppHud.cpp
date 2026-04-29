#include "CloudSeamanor/app/GameAppHud.hpp"

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CloudGuardianContract.hpp"
#include "CloudSeamanor/domain/CropData.hpp"
#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/app/GameAppFarming.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/infrastructure/GameConfig.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/domain/Player.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"

#include <algorithm>
#include <array>
#include <sstream>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::rendering::toSfString;
struct NewbieGoalView {
    std::string title;
    std::array<std::string, 3> items{};
};

const CloudSeamanor::infrastructure::GameConfig& NewbieGoalsConfig_() {
    static CloudSeamanor::infrastructure::GameConfig cfg;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        (void)cfg.LoadFromFile("configs/newbie_7day_goals.cfg");
    }
    return cfg;
}

NewbieGoalView BuildNewbieGoalView_(
    int day,
    const std::array<std::string, 3>& fallback_recommendations) {
    const auto& cfg = NewbieGoalsConfig_();
    const int day_bucket = std::clamp(day, 1, 7);
    const std::string day_key = "newbie_day" + std::to_string(day_bucket);

    NewbieGoalView view;
    view.title = cfg.GetString(day_key + "_title", "三件事推荐：");
    view.items[0] = cfg.GetString(day_key + "_goal_1", fallback_recommendations[0]);
    view.items[1] = cfg.GetString(day_key + "_goal_2", fallback_recommendations[1]);
    view.items[2] = cfg.GetString(day_key + "_goal_3", fallback_recommendations[2]);
    return view;
}


struct PlotStatusSummary {
    int ready_plots = 0;
    int dry_plots = 0;
    int growing_plots = 0;
    int tea_garden_plots = 0;
    int normal_farm_plots = 0;
};

PlotStatusSummary BuildPlotStatusSummary(const std::vector<TeaPlot>& tea_plots) {
    PlotStatusSummary summary;
    for (const auto& plot : tea_plots) {
        if (plot.ready) {
            ++summary.ready_plots;
        } else if (plot.seeded && !plot.watered) {
            ++summary.dry_plots;
        } else if (plot.seeded) {
            ++summary.growing_plots;
        }
        if (plot.layer == TeaPlotLayer::TeaGardenExclusive) {
            ++summary.tea_garden_plots;
        } else {
            ++summary.normal_farm_plots;
        }
    }
    return summary;
}

std::string BuildInventorySummaryText(const CloudSeamanor::domain::Inventory& inventory,
                                      const RepairProject& main_house_repair,
                                      const TeaMachine& tea_machine,
                                      const SpiritBeast& spirit_beast,
                                      bool spirit_beast_watered_today,
                                      const CloudSeamanor::domain::WorkshopSystem* workshop) {
    bool workshop_running = false;
    float workshop_progress = 0.0f;
    if (workshop) {
        if (const auto* machine = workshop->GetMachine("tea_machine")) {
            workshop_running = machine->is_processing;
            workshop_progress = machine->progress;
        }
    }

    const int tea_seed = inventory.CountOf("TeaSeed");
    const int turnip_seed = inventory.CountOf("TurnipSeed");
    const int tea_leaf = inventory.CountOf("TeaLeaf");
    const int turnip = inventory.CountOf("Turnip");
    const int tea_pack = inventory.CountOf("TeaPack");
    const int wood = inventory.CountOf("Wood");
    const bool inventory_empty = (tea_seed + turnip_seed + tea_leaf + turnip + tea_pack + wood) <= 0;

    std::ostringstream inventory_stream;
    inventory_stream << "【背包】";
    if (inventory_empty) {
        inventory_stream << "\n空背包（先去采集或收获）";
    } else {
        inventory_stream << "\n作物："
                         << ItemDisplayName("TeaLeaf") << " x" << tea_leaf
                         << "，" << ItemDisplayName("Turnip") << " x" << turnip
                         << "，" << ItemDisplayName("TeaPack") << " x" << tea_pack
                         << "\n资源：" << ItemDisplayName("Wood") << " x" << wood
                         << "\n种子：" << ItemDisplayName("TeaSeed") << " x" << tea_seed
                         << "，" << ItemDisplayName("TurnipSeed") << " x" << turnip_seed;
    }

    const std::string house_stage = [&]() {
        switch (main_house_repair.level) {
        case 1: return std::string("帐篷");
        case 2: return std::string("小木屋");
        case 3: return std::string("砖房");
        case 4: return std::string("大宅");
        default: return std::string("未知");
        }
    }();
    inventory_stream << "\n\n主屋：Lv." << main_house_repair.level << "（" << house_stage << "）"
                     << "\n制茶机：" << (workshop_running ? "加工中 " + std::to_string(static_cast<int>(workshop_progress)) + "%" : "空闲")
                     << "\n待领取：" << tea_machine.queued_output
                     << "\n灵兽结缘：" << (spirit_beast.daily_interacted ? "已完成" : "可进行")
                     << "\n灵兽名：" << spirit_beast.custom_name
                     << "（" << SpiritBeastPersonalityText(spirit_beast.personality) << "）"
                     << "\n灵兽羁绊：" << spirit_beast.favor << "/100"
                     << "  派遣：" << (spirit_beast.dispatched_for_pest_control ? "自动除虫中" : "未派遣")
                     << "\n灵兽协助：" << (spirit_beast_watered_today ? "已使用" : "可使用");
    return inventory_stream.str();
}

std::string BuildWorldTipText(const std::string& current_target_text,
                              int highlighted_npc_index,
                              int highlighted_plot_index,
                              const std::vector<NpcActor>& npcs,
                              const std::vector<TeaPlot>& tea_plots,
                              const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life) {
    std::string world_tip = current_target_text;
    if (highlighted_npc_index >= 0) {
        const auto& npc = npcs[static_cast<std::size_t>(highlighted_npc_index)];
        std::string life_stage_text;
        if (dynamic_life) {
            life_stage_text = " | " + dynamic_life->GetNpcStageText(npc.id);
        }
        world_tip = npc.display_name + " | 好感 " + NpcHeartText(npc.heart_level) +
                    " (" + std::to_string(npc.favor) + ")" +
                    life_stage_text +
                    " | 今日赠礼：" + (npc.daily_gifted ? std::string("已完成") : std::string("可进行"));
    } else if (highlighted_plot_index >= 0) {
        const auto& plot = tea_plots[static_cast<std::size_t>(highlighted_plot_index)];
        const std::string layer_text =
            (plot.layer == TeaPlotLayer::TeaGardenExclusive) ? "茶园专属" : "普通农田";
        const std::string quality_prefix =
            CloudSeamanor::domain::CropTable::QualityToText(plot.quality);
        const std::string quality_line =
            (plot.quality != CloudSeamanor::domain::CropQuality::Normal)
                ? " | 品质：" + std::string(quality_prefix) + " x" +
                  std::to_string(CloudSeamanor::domain::CropTable::QualityHarvestMultiplier(plot.quality)) + "产"
                : "";
        world_tip = plot.crop_name + " | " + layer_text + " | " + PlotStatusText(plot) +
                    " | 种子 " + ItemDisplayName(plot.seed_item_id) + " -> 收获 " +
                    ItemDisplayName(plot.harvest_item_id) + quality_line;
    }
    return world_tip;
}

std::string BuildDialogueOverlayText(const std::vector<NpcActor>& npcs,
                                     int highlighted_npc_index,
                                     const std::string& dialogue_text,
                                     const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life) {
    const std::string npc_hint = BuildNpcInteractionHint();
    if (highlighted_npc_index >= 0) {
        const auto& npc = npcs[static_cast<std::size_t>(highlighted_npc_index)];
        std::string life_stage_line;
        if (dynamic_life) {
            life_stage_line = dynamic_life->GetNpcStageText(npc.id);
        }
        std::string dialogue_title = "对话 | " + npc.display_name;
        dialogue_title += " | 云形态 " + NpcCloudStageText(npc.heart_level);
        if (!life_stage_line.empty()) {
            dialogue_title += " | " + life_stage_line;
        }
        return dialogue_title + "\n" + dialogue_text + "\n" + npc_hint;
    }
    return "对话\n" + dialogue_text + "\n" + npc_hint;
}
} // namespace

void UpdateHudText(bool font_loaded,
                   sf::Text& hud_text,
                   sf::Text& inventory_text,
                   sf::Text& hint_text,
                   sf::Text& dialogue_overlay_text,
                   sf::Text& debug_text,
                   sf::Text& world_tip_text,
                   const CloudSeamanor::domain::GameClock& clock,
                   const CloudSeamanor::domain::CloudSystem& cloud_system,
                   const CloudSeamanor::domain::StaminaSystem& stamina,
                   const CloudSeamanor::domain::Player& player,
                   const CloudSeamanor::domain::Inventory& inventory,
                   const std::vector<TeaPlot>& tea_plots,
                   const RepairProject& main_house_repair,
                   const TeaMachine& tea_machine,
                   const SpiritBeast& spirit_beast,
                   bool spirit_beast_watered_today,
                   const std::vector<NpcActor>& npcs,
                   int highlighted_plot_index,
                   int highlighted_npc_index,
                   const std::string& hint_message,
                   float session_time,
                   const std::string& current_target_text,
                   const std::string& dialogue_text,
                   std::size_t pickup_count,
                   const CloudSeamanor::domain::SkillSystem* skills,
                   const CloudSeamanor::domain::FestivalSystem* festivals,
                   const std::string& festival_notice_text,
                   const CloudSeamanor::domain::DynamicLifeSystem* dynamic_life,
                   const CloudSeamanor::domain::WorkshopSystem* workshop) {
    if (!font_loaded) {
        return;
    }

    const PlotStatusSummary plot_summary = BuildPlotStatusSummary(tea_plots);

    const auto base_recommendations = BuildDailyRecommendations(
        clock,
        cloud_system.CurrentState(),
        cloud_system,
        main_house_repair,
        inventory,
        tea_machine,
        spirit_beast,
        spirit_beast_watered_today,
        tea_plots,
        npcs);
    const std::array<std::string, 3> fallback_recommendations = {
        base_recommendations[0], base_recommendations[1], base_recommendations[2]};
    const NewbieGoalView newbie_goals = BuildNewbieGoalView_(clock.Day(), fallback_recommendations);

    std::ostringstream hud_stream;
    hud_stream << "第 " << clock.Day() << " 天"
               << "  " << clock.TimeText()
               << "  " << PhaseText(clock.CurrentPhase())
               << "\n云海：" << cloud_system.CurrentStateText()
               << "  明日预报：" << cloud_system.ForecastStateText()
               << "  今日加成 x" << CloudGrowthMultiplier(cloud_system.CurrentState())
               << "\n灵气 " << cloud_system.SpiritEnergy()
               << "  体力 " << static_cast<int>(stamina.Current()) << "/" << static_cast<int>(stamina.Max())
               << "  朝向 " << player.FacingText()
               << "\n可收获地块 " << plot_summary.ready_plots
               << "  缺水 " << plot_summary.dry_plots
               << "  生长中 " << plot_summary.growing_plots
               << "\n茶园地块 " << plot_summary.tea_garden_plots
               << "  普通农田 " << plot_summary.normal_farm_plots
               << "\n目标对象：" << current_target_text
               << "\n今日目标：" << BuildDailyGoalText(main_house_repair, inventory, tea_machine, spirit_beast, npcs)
               << "\n" << newbie_goals.title
               << "\n1. " << newbie_goals.items[0]
               << "\n2. " << newbie_goals.items[1]
               << "\n3. " << newbie_goals.items[2]
               << "\n天气建议：" << BuildWeatherAdviceText(cloud_system.CurrentState(), cloud_system.IsForecastVisible());
    if (skills) {
        hud_stream << "\n--- 技能成长 ---";
        hud_stream << "\n" << skills->GetLevelText(CloudSeamanor::domain::SkillType::SpiritFarm)
                   << " " << static_cast<int>(skills->GetExpRatio(CloudSeamanor::domain::SkillType::SpiritFarm) * 100) << "%";
        hud_stream << " | " << skills->GetLevelText(CloudSeamanor::domain::SkillType::SpiritForage)
                   << " " << static_cast<int>(skills->GetExpRatio(CloudSeamanor::domain::SkillType::SpiritForage) * 100) << "%";
    }
    if (festivals) {
        const std::string fest_notice = festivals->GetNoticeText();
        if (!fest_notice.empty()) {
            hud_stream << "\n节日预告：" << fest_notice;
        }
    }
    if (!festival_notice_text.empty()) {
        hud_stream << "\n今日节日：" << festival_notice_text;
    }
    hud_text.setString(toSfString(hud_stream.str()));

    inventory_text.setString(toSfString(BuildInventorySummaryText(
        inventory,
        main_house_repair,
        tea_machine,
        spirit_beast,
        spirit_beast_watered_today,
        workshop)));

    std::ostringstream debug_stream;
    debug_stream << "调试面板 [F3]\n"
                 << "本局时长：" << static_cast<int>(session_time) << "秒"
                 << "\n玩家坐标：" << static_cast<int>(player.GetPosition().x) << ", " << static_cast<int>(player.GetPosition().y)
                 << "\n掉落物：" << pickup_count
                 << "  NPC 目标：" << (highlighted_npc_index >= 0 ? npcs[static_cast<std::size_t>(highlighted_npc_index)].display_name : std::string("无"))
                 << "\n灵兽状态：" << SpiritBeastStateText(spirit_beast.state)
                 << "  巡逻点索引：" << spirit_beast.patrol_index;
    if (dynamic_life && highlighted_npc_index >= 0) {
        const auto& npc = npcs[static_cast<std::size_t>(highlighted_npc_index)];
        debug_stream << "\n【人生系统】" << dynamic_life->GetNpcStageText(npc.id);
    }
    debug_text.setString(toSfString(debug_stream.str()));

    const std::string world_tip = BuildWorldTipText(
        current_target_text,
        highlighted_npc_index,
        highlighted_plot_index,
        npcs,
        tea_plots,
        dynamic_life);

    auto& contract = CloudSeamanor::domain::GetGlobalContract();
    const int completed_pacts = contract.CompletedPactCount();
    if (const auto* tracking = contract.GetTrackingVolume();
        tracking && (completed_pacts > 0 || clock.Day() > 3)) {
        std::string contract_line = "[契约] " + contract.ProgressText() +
            " | 追踪：" + tracking->name +
            "：" + contract.TodayRecommendation();
        world_tip_text.setString(toSfString(world_tip + "\n" + contract_line));
    } else {
        world_tip_text.setString(toSfString(world_tip));
    }

    const std::string controls_hint = BuildControlsHint();
    const std::string no_pressure_hint = "无硬性期限，错过内容可后续再体验";
    const std::string hint_full = hint_message.empty()
        ? controls_hint + " | " + no_pressure_hint
        : hint_message + " | " + controls_hint + " | " + no_pressure_hint;
    hint_text.setString(toSfString(hint_full));

    dialogue_overlay_text.setString(toSfString(BuildDialogueOverlayText(
        npcs,
        highlighted_npc_index,
        dialogue_text,
        dynamic_life)));
}

void RefreshWindowTitle(sf::RenderWindow& window,
                        const CloudSeamanor::domain::GameClock& clock,
                        const CloudSeamanor::domain::CloudSystem& cloud_system,
                        const CloudSeamanor::domain::Player& player,
                        const CloudSeamanor::domain::StaminaSystem& stamina,
                        const RepairProject& main_house_repair,
                        const TeaMachine& tea_machine,
                        const SpiritBeast& spirit_beast,
                        const std::vector<NpcActor>& npcs,
                        const std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                        int highlighted_index,
                        const std::string& current_target_text,
                        bool can_sleep_now,
                        const CloudSeamanor::domain::SkillSystem* skills,
                        const CloudSeamanor::domain::FestivalSystem* festivals) {
    const std::string interaction_text = highlighted_index >= 0
        ? " | 目标：" + current_target_text + " | 按 E"
        : " | 目标：无";

    const std::string sleep_text = can_sleep_now
        ? " | 休息：按 T"
        : "";

    const std::string forecast_text = cloud_system.IsForecastVisible()
        ? " | 明日：" + cloud_system.ForecastStateText()
        : " | 明日：22:00后公布";

    (void)festivals;

    std::string skill_text;
    if (skills) {
        skill_text = " | 灵农Lv." + std::to_string(skills->GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm));
    }

    std::string title = "云海山庄 | " + clock.DateText() + " " + clock.TimeText() +
        " | 云海：" + cloud_system.CurrentStateText() +
        forecast_text +
        " | 今日加成 x" + std::to_string(CloudGrowthMultiplier(cloud_system.CurrentState())) +
        " | 灵气 " + std::to_string(cloud_system.SpiritEnergy()) +
        skill_text +
        " | 主屋：" + std::string(main_house_repair.completed ? "已修复" : "破损") +
        " | 制茶机：" + std::string(tea_machine.running ? "运行中" : "空闲") +
        " | NPC 数量：" + std::to_string(static_cast<int>(npcs.size())) +
        " | 灵兽：" + std::string(SpiritBeastStateText(spirit_beast.state)) +
        " | 结缘：" + std::string(spirit_beast.daily_interacted ? "已完成" : "可进行") +
        " | 朝向：" + player.FacingText() +
        " | 体力：" + std::to_string(static_cast<int>(stamina.Current())) +
        interaction_text +
        sleep_text +
        " | 掉落物：" + std::to_string(static_cast<int>(pickups.size())) +
        " | 无硬性期限";
    window.setTitle(toSfString(title));
}

} // namespace CloudSeamanor::engine
