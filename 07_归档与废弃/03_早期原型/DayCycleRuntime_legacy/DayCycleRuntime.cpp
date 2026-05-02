#include "CloudSeamanor/DayCycleRuntime.hpp"

// [ARCHIVED 2026-04-28]
// Legacy implementation moved out of active build.

#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

namespace {
std::string SeasonalBgmPath(CloudSeamanor::domain::Season season) {
    using CloudSeamanor::domain::Season;
    switch (season) {
    case Season::Spring: return "assets/audio/bgm/spring_theme.wav";
    case Season::Summer: return "assets/audio/bgm/summer_theme.wav";
    case Season::Autumn: return "assets/audio/bgm/autumn_theme.wav";
    case Season::Winter: return "assets/audio/bgm/winter_theme.wav";
    }
    return "assets/audio/bgm/spring_theme.wav";
}

int BuildDailyInfluence(const std::vector<TeaPlot>& tea_plots,
                        bool main_house_repaired,
                        bool spirit_beast_interacted,
                        bool include_beast_penalty) {
    int daily_influence = 0;
    for (const auto& plot : tea_plots) {
        if (plot.seeded && plot.watered) daily_influence += 5;
    }
    if (!main_house_repaired) daily_influence += 5;
    if (include_beast_penalty && !spirit_beast_interacted) daily_influence -= 3;
    return daily_influence;
}

void ResetDailyFlags(DayCycleRuntimeContext& context) {
    for (auto& plot : context.tea_plots) {
        plot.watered = false;
        context.refresh_plot_visual(plot, false);
    }
    context.spirit_beast.daily_interacted = false;
    context.spirit_beast.last_interaction_day = context.clock.Day();
    context.spirit_beast_watered_today = false;
    for (auto& npc : context.npcs) {
        npc.daily_gifted = false;
        npc.daily_talked = false;
    }
}

void UpdateFestivalNotice(DayCycleRuntimeContext& context) {
    const std::string festival_notice = context.festivals.GetNoticeText();
    context.festival_notice_text = festival_notice;
    if (const auto* today = context.festivals.GetTodayFestival()) {
        const std::string notice = "今天是「" + today->name + "」！" + today->activity + "，" + today->description + "。";
        context.push_hint(notice + "参与可获得：" + today->reward + "。", 5.0f);
        context.log_info("节日到来：" + today->name);
    }
}

} // namespace

void HandleNaturalDayTransition(DayCycleRuntimeContext& context, int previous_day) {
    if (context.clock.Day() == previous_day) {
        return;
    }

    if (context.delegate_on_day_changed) {
        context.delegate_on_day_changed();
        return;
    }
    if (context.log_info) {
        context.log_info("DayCycleRuntime fallback path is active. Prefer delegating to GameRuntime::OnDayChanged().");
    }

    context.cloud_system.AdvanceToNextDay(context.clock.Day());
    const int daily_influence = BuildDailyInfluence(
        context.tea_plots,
        context.main_house_repair.completed,
        context.spirit_beast.daily_interacted,
        true
    );
    context.cloud_system.ApplyPlayerInfluence(daily_influence);
    context.contract.CheckVolumeUnlocks();

    const bool is_season_start = (context.clock.Day() % 28 == 1);
    const bool is_year_start = (context.clock.Day() == 1);
    const float cloud_density = context.cloud_system.CurrentSpiritDensity();

    context.festivals.Update(context.clock.Season(), context.clock.DayInSeason());
    UpdateFestivalNotice(context);

    context.dynamic_life.UpdateDaily(cloud_density);
    context.dynamic_life.CheckStageTransitions(is_season_start, is_year_start);
    for (auto& npc : context.npcs) {
        if (npc.favor >= GameConstants::Npc::FavorDailyThreshold) {
            context.dynamic_life.AddPlayerPoints(npc.id, static_cast<float>(npc.favor) * GameConstants::Npc::FavorDailyToDynamicLifeMultiplier);
        }
        if (npc.daily_gifted) {
            context.dynamic_life.AddPlayerPoints(npc.id, GameConstants::Npc::DailyGiftDynamicLifePoints);
        }
    }

    ResetDailyFlags(context);
    context.run_spirit_beast_watering_aid();
    const std::string spirit_gain_text = " 灵气 +" + std::to_string(context.cloud_system.SpiritEnergyGain()) + "。";
    const std::string contract_text = context.contract.CompletedVolumeCount() > 0
        ? " 契约进度：" + std::to_string(context.contract.CompletedVolumeCount()) + "卷已完成。"
        : "";
    context.push_hint("新的一天开始了。每日行动已刷新，昨天没做完的事情今天也能继续。" + spirit_gain_text + contract_text, 5.2f);
    context.push_hint("BGM 切换：" + SeasonalBgmPath(context.clock.Season()), 1.4f);
    const std::string ambient = context.cloud_system.AmbientSfxId();
    if (!ambient.empty()) {
        context.push_hint("环境音效：" + ambient, 1.2f);
    }
    context.save_game();
    context.log_info("新的一天已经开始。");
}

void SleepToNextMorning(DayCycleRuntimeContext& context) {
    if (context.delegate_sleep_to_next_morning) {
        context.delegate_sleep_to_next_morning();
        return;
    }
    if (context.log_info) {
        context.log_info("DayCycleRuntime fallback path is active. Prefer delegating to GameRuntime::SleepToNextMorning().");
    }

    context.cloud_system.AdvanceToNextDay(context.clock.Day());
    const int daily_influence = BuildDailyInfluence(
        context.tea_plots,
        context.main_house_repair.completed,
        context.spirit_beast.daily_interacted,
        false
    );
    context.cloud_system.ApplyPlayerInfluence(daily_influence);
    context.cloud_system.UpdateForecastVisibility(context.clock.Day(), context.clock.Hour());
    context.clock.SleepToNextMorning();

    const bool is_season_start = (context.clock.Day() % 28 == 1);
    const bool is_year_start = (context.clock.Day() == 1);
    const float cloud_density = context.cloud_system.CurrentSpiritDensity();
    context.festivals.Update(context.clock.Season(), context.clock.DayInSeason());
    UpdateFestivalNotice(context);

    std::unordered_map<std::string, int> outputs;
    context.workshop.Update(0.0f, cloud_density, outputs);

    context.dynamic_life.UpdateDaily(cloud_density);
    context.dynamic_life.CheckStageTransitions(is_season_start, is_year_start);
    for (auto& npc : context.npcs) {
        if (npc.favor >= 4) {
            context.dynamic_life.AddPlayerPoints(npc.id, static_cast<float>(npc.favor) * 2.0f);
        }
    }

    context.stamina.Refill();
    ResetDailyFlags(context);
    context.run_spirit_beast_watering_aid();
    const std::string spirit_gain_text = " 灵气 +" + std::to_string(context.cloud_system.SpiritEnergyGain()) + "。";
    context.push_hint("你醒来时精神焕发。每日行动与作物浇水状态已重置；没完成的事明天还能继续。" + spirit_gain_text, 3.8f);
    context.save_game();
    context.log_info("Player slept until next morning.");
    context.update_highlighted_interactable();
    context.update_hud_text();
}

} // namespace CloudSeamanor::engine
