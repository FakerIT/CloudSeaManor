#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"

#include "CloudSeamanor/FarmingLogic.hpp"

#include <algorithm>
#include <cstdlib>

namespace CloudSeamanor::engine {

// ============================================================================
// 【CropGrowthSystem::CropGrowthSystem】构造函数
// ============================================================================
CropGrowthSystem::CropGrowthSystem() = default;

// ============================================================================
// 【CropGrowthSystem::SetHintCallback】设置提示回调
// ============================================================================
void CropGrowthSystem::SetHintCallback(HintCallback callback) {
    hint_callback_ = std::move(callback);
}

// ============================================================================
// 【CropGrowthSystem::Update】每帧更新作物生长
// ============================================================================
void CropGrowthSystem::Update(
    GameWorldState& world_state,
    float delta_seconds
) {
    for (auto& plot : world_state.GetTeaPlots()) {
        if (!plot.cleared) continue;
        if (!plot.seeded || (!plot.watered && !plot.sprinkler_installed)) continue;
        if (plot.ready) continue;

        const int prev_stage = plot.stage;
        float growth_multiplier = 1.0f;
        if (plot.fertilized) {
            growth_multiplier *= (plot.fertilizer_type == "premium")
                ? fertilizer_premium_multiplier_
                : fertilizer_basic_multiplier_;
        }
        if (plot.in_greenhouse) {
            growth_multiplier *= 1.1f;
        }
        plot.growth += delta_seconds * growth_multiplier;

        const int new_stage = std::clamp(
            static_cast<int>((plot.growth_time > 0.0f)
                                 ? (plot.growth / plot.growth_time * static_cast<float>(plot.growth_stages))
                                 : static_cast<float>(plot.growth_stages)),
            0,
            plot.growth_stages);
        plot.stage = std::max(plot.stage, new_stage);

        if (plot.stage != prev_stage && plot.stage >= plot.growth_stages) {
            plot.ready = true;
            if (hint_callback_) {
                hint_callback_("作物已进入收获阶段！", 2.5f);
            }
        }
    }
}

void CropGrowthSystem::HandleSeasonChanged(
    GameWorldState& world_state,
    CloudSeamanor::domain::Season current_season
) {
    for (auto& plot : world_state.GetTeaPlots()) {
        if (!ShouldPlotWiltBecauseSeason(plot, current_season)) {
            continue;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;
        if (hint_callback_) {
            hint_callback_("季节已过，作物枯萎了：" + plot.crop_name + "。", 2.2f);
        }
    }
}

void CropGrowthSystem::HandleDailyDiseaseAndPest(GameWorldState& world_state) {
    constexpr int kBaseInfectChance = 10;
    constexpr int kWateredInfectChance = 6;
    constexpr int kAutoClearChanceHighFavor = 30;
    constexpr int kAutoClearChanceDispatched = 70;

    auto& beast = world_state.GetSpiritBeast();
    const bool high_favor_beast = beast.favor >= 60;

    for (auto& plot : world_state.GetTeaPlots()) {
        if (!plot.cleared || !plot.seeded || plot.ready) {
            continue;
        }

        const int infect_chance = plot.watered ? kWateredInfectChance : kBaseInfectChance;
        const bool has_issue_before = plot.disease || plot.pest;

        if (!plot.disease && (std::rand() % 100) < infect_chance) {
            plot.disease = true;
            if (hint_callback_) {
                hint_callback_("作物感染了虫害！", 2.2f);
            }
        }
        if (!plot.pest && (std::rand() % 100) < infect_chance) {
            plot.pest = true;
            if (hint_callback_) {
                hint_callback_("作物感染了虫害！", 2.2f);
            }
        }

        if (plot.disease || plot.pest) {
            ++plot.disease_days;
        } else if (has_issue_before) {
            plot.disease_days = 0;
        }

        int auto_clear_chance = 0;
        if (high_favor_beast) {
            auto_clear_chance = std::max(auto_clear_chance, kAutoClearChanceHighFavor);
        }
        if (beast.dispatched_for_pest_control) {
            auto_clear_chance = std::max(auto_clear_chance, kAutoClearChanceDispatched);
        }
        if ((plot.disease || plot.pest)
            && auto_clear_chance > 0
            && (std::rand() % 100) < auto_clear_chance) {
            plot.disease = false;
            plot.pest = false;
            plot.disease_days = 0;
            if (hint_callback_) {
                hint_callback_(
                    beast.dispatched_for_pest_control
                        ? "派遣灵兽已自动清除地块病虫害。"
                        : "高好感灵兽协助清除了地块病虫害。",
                    2.2f);
            }
        }
    }
}

}  // namespace CloudSeamanor::engine
