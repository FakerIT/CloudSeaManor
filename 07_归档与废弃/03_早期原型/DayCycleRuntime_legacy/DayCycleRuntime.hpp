#pragma once

// [ARCHIVED 2026-04-28]
// DayCycleRuntime 已被 GameRuntime 单入口（日切/睡眠）替代。
// 该文件仅用于历史归档，不再参与编译。

// ============================================================================
// 【DayCycleRuntime.hpp】跨日运行时
// ============================================================================
// 提供跨日刷新逻辑所需的上下文结构体和函数声明。
// ... (legacy content kept for historical reference)

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

struct DayCycleRuntimeContext {
    CloudSeamanor::domain::GameClock& clock;
    CloudSeamanor::domain::CloudSystem& cloud_system;
    CloudSeamanor::domain::CloudGuardianContract& contract;
    CloudSeamanor::domain::StaminaSystem& stamina;
    RepairProject& main_house_repair;
    std::vector<TeaPlot>& tea_plots;
    SpiritBeast& spirit_beast;
    bool& spirit_beast_watered_today;
    std::vector<NpcActor>& npcs;
    CloudSeamanor::domain::FestivalSystem& festivals;
    CloudSeamanor::domain::WorkshopSystem& workshop;
    CloudSeamanor::domain::DynamicLifeSystem& dynamic_life;
    std::string& festival_notice_text;
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
    std::function<void()> save_game;
    std::function<void()> update_highlighted_interactable;
    std::function<void()> update_hud_text;
    std::function<void(TeaPlot&, bool)> refresh_plot_visual;
    std::function<void()> run_spirit_beast_watering_aid;
    std::function<void()> delegate_on_day_changed = nullptr;
    std::function<void()> delegate_sleep_to_next_morning = nullptr;

    DayCycleRuntimeContext(
        CloudSeamanor::domain::GameClock& clk,
        CloudSeamanor::domain::CloudSystem& cloud,
        CloudSeamanor::domain::CloudGuardianContract& con,
        CloudSeamanor::domain::StaminaSystem& sta,
        RepairProject& repair,
        std::vector<TeaPlot>& plots,
        SpiritBeast& beast,
        bool& beast_watered,
        std::vector<NpcActor>& npcs_list,
        CloudSeamanor::domain::FestivalSystem& fest,
        CloudSeamanor::domain::WorkshopSystem& ws,
        CloudSeamanor::domain::DynamicLifeSystem& dl,
        std::string& fest_notice,
        std::function<void(const std::string&, float)> hint_fn,
        std::function<void(const std::string&)> log_fn,
        std::function<void()> save_fn,
        std::function<void()> hud_fn,
        std::function<void(TeaPlot&, bool)> plot_fn,
        std::function<void()> beast_fn,
        std::function<void()> highlight_fn = nullptr
    ) : clock(clk),
        cloud_system(cloud),
        contract(con),
        stamina(sta),
        main_house_repair(repair),
        tea_plots(plots),
        spirit_beast(beast),
        spirit_beast_watered_today(beast_watered),
        npcs(npcs_list),
        festivals(fest),
        workshop(ws),
        dynamic_life(dl),
        festival_notice_text(fest_notice),
        push_hint(hint_fn),
        log_info(log_fn),
        save_game(save_fn),
        update_hud_text(hud_fn),
        refresh_plot_visual(plot_fn),
        run_spirit_beast_watering_aid(beast_fn),
        update_highlighted_interactable(highlight_fn)
    {}
};

[[deprecated("Use GameRuntime::OnDayChanged() as the single daily-entry source.")]]
void HandleNaturalDayTransition(DayCycleRuntimeContext& context, int previous_day);

[[deprecated("Use GameRuntime::SleepToNextMorning() as the single sleep-entry source.")]]
void SleepToNextMorning(DayCycleRuntimeContext& context);

} // namespace CloudSeamanor::engine
