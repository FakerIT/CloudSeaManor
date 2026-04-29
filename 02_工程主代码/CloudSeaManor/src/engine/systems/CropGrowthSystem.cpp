#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"

#include "CloudSeamanor/engine/FarmingLogic.hpp"
#include "CloudSeamanor/domain/CropData.hpp"

#include <algorithm>
#include <cstdlib>

namespace CloudSeamanor::engine {

namespace {

float ApplyNightGrowthPenalty_(const float growth_multiplier, const int hour) {
    if (hour >= 22 || hour < 6) {
        return growth_multiplier * 0.5f;
    }
    return growth_multiplier;
}

float TeaGardenUpgradeMultiplier_(const int main_house_level) {
    if (main_house_level >= 4) return 1.30f;
    if (main_house_level == 3) return 1.20f;
    if (main_house_level == 2) return 1.10f;
    return 1.00f;
}

}  // namespace

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
// 【CropGrowthSystem::CalculateGrowthMultiplier】计算生长倍率
// ============================================================================
float CropGrowthSystem::CalculateGrowthMultiplier(
    const TeaPlot& plot,
    const CloudSeamanor::domain::CloudState cloud_state
) {
    float growth_multiplier = 1.0f;

    // 肥料加成
    if (plot.fertilized) {
        if (plot.fertilizer_type == "tea_soul") {
            growth_multiplier *= fertilizer_tea_soul_multiplier_;
        } else if (plot.fertilizer_type == "cloud_essence") {
            growth_multiplier *= fertilizer_cloud_multiplier_;
        } else if (plot.fertilizer_type == "spirit") {
            growth_multiplier *= fertilizer_spirit_multiplier_;
        } else if (plot.fertilizer_type == "premium") {
            growth_multiplier *= fertilizer_premium_multiplier_;
        } else {
            growth_multiplier *= fertilizer_basic_multiplier_;
        }
    }

    // 温室加成
    if (plot.in_greenhouse) {
        growth_multiplier *= 1.1f;
    }

    // 洒水器加成
    if (plot.sprinkler_installed) {
        growth_multiplier *= 1.05f;
    }

    // 云海天气加成（白天生效）
    switch (cloud_state) {
    case CloudSeamanor::domain::CloudState::Clear:
        break;  // 1.0x
    case CloudSeamanor::domain::CloudState::Mist:
        growth_multiplier *= 1.15f;
        break;
    case CloudSeamanor::domain::CloudState::DenseCloud:
        growth_multiplier *= 1.40f;
        break;
    case CloudSeamanor::domain::CloudState::Tide:
        growth_multiplier *= 1.60f;
        break;
    }

    return growth_multiplier;
}

// ============================================================================
// 【CropGrowthSystem::Update】每帧更新作物生长
// ============================================================================
void CropGrowthSystem::Update(
    GameWorldState& world_state,
    const float delta_seconds,
    const CloudSeamanor::domain::CloudSystem& cloud_system
) {
    const auto cloud_state = cloud_system.CurrentState();

    for (auto& plot : world_state.MutableTeaPlots()) {
        if (!plot.cleared) continue;
        if (!plot.seeded || (!plot.watered && !plot.sprinkler_installed)) continue;
        if (plot.ready) continue;

        // 病虫害惩罚
        if (plot.disease || plot.pest) {
            if (hint_callback_) {
                hint_callback_("作物受病虫害影响，生长变慢！", 2.0f);
            }
            continue;
        }

        const int prev_stage = plot.stage;

        // 生长倍率
        float growth_multiplier = CalculateGrowthMultiplier(plot, cloud_state);

        if (plot.layer == TeaPlotLayer::TeaGardenExclusive) {
            growth_multiplier *= TeaGardenUpgradeMultiplier_(world_state.GetMainHouseRepair().level);
        }

        // 夜晚（22:00~6:00）生长减半（统一入口，避免分散在其他系统）
        const int hour = world_state.GetClock().Hour();
        growth_multiplier = ApplyNightGrowthPenalty_(growth_multiplier, hour);

        plot.growth += delta_seconds * growth_multiplier;

        const int new_stage = std::clamp(
            (plot.growth_time > 0.0f)
                ? static_cast<int>(plot.growth / plot.growth_time * static_cast<float>(plot.growth_stages))
                : plot.growth_stages,
            0,
            plot.growth_stages);
        plot.stage = std::max(plot.stage, new_stage);

        if (plot.stage != prev_stage && plot.stage >= plot.growth_stages) {
            plot.ready = true;
            // 成熟时检测灵化变种
            CheckSpiritMutation(plot, cloud_system);
            if (hint_callback_) {
                hint_callback_("作物已进入收获阶段！", 2.5f);
            }
        }
    }
}

// ============================================================================
// 【CropGrowthSystem::HandleDailyAccumulation】每日累积云海天数
// ============================================================================
void CropGrowthSystem::HandleDailyAccumulation(
    GameWorldState&,
    const CloudSeamanor::domain::CloudSystem& cloud_system
) {
    const auto state = cloud_system.CurrentState();

    // 注：此方法由外部在 SleepToNextMorning 时对所有地块调用
    // 这里仅提供静态入口，具体地块更新在调用方遍历
    (void)cloud_system;  // suppress unused warning
}

// ============================================================================
// 【CropGrowthSystem::ApplyPlantingSnapshot】播种时记录品质快照
// ============================================================================
void CropGrowthSystem::ApplyPlantingSnapshot(
    TeaPlot& plot,
    const CloudSeamanor::domain::CloudSystem& cloud_system
) {
    plot.cloud_density_at_planting = cloud_system.CurrentSpiritDensity();
    plot.aura_at_planting = cloud_system.SpiritEnergy();
    plot.weather_at_planting = cloud_system.CurrentState();
    plot.dense_cloud_days_accumulated = 0;
    plot.tide_days_accumulated = 0;
    // Keep quality fertilizer snapshot in sync with runtime fertilizer state.
    plot.fertilizer_type_for_quality = plot.fertilizer_type;
    plot.spirit_mutated = false;
}

// ============================================================================
// 【CropGrowthSystem::CheckSpiritMutation】检测灵化变种
// ============================================================================
void CropGrowthSystem::CheckSpiritMutation(
    TeaPlot& plot,
    const CloudSeamanor::domain::CloudSystem& cloud_system
) {
    if (cloud_system.IsDenseOrTide() && !plot.spirit_mutated) {
        // 浓云海 15%，大潮 30%
        const float mutate_chance = cloud_system.IsTide() ? 0.30f : 0.15f;
        const float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        if (roll < mutate_chance) {
            plot.spirit_mutated = true;
            plot.harvest_amount = static_cast<int>(plot.harvest_amount * 2.0f);
            if (hint_callback_) {
                hint_callback_("灵化变种！产量翻倍！", 3.0f);
            }
        }
    }
}

// ============================================================================
// 【CropGrowthSystem::HandleSeasonChanged】季节变化处理
// ============================================================================
void CropGrowthSystem::HandleSeasonChanged(
    GameWorldState& world_state,
    CloudSeamanor::domain::Season current_season
) {
    for (auto& plot : world_state.MutableTeaPlots()) {
        if (!ShouldPlotWiltBecauseSeason(plot, current_season)) {
            continue;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;
        plot.spirit_mutated = false;
        plot.dense_cloud_days_accumulated = 0;
        plot.tide_days_accumulated = 0;
        if (hint_callback_) {
            hint_callback_("季节已过，作物枯萎了：" + plot.crop_name + "。", 2.2f);
        }
    }
}

// ============================================================================
// 【CropGrowthSystem::HandleDailyDiseaseAndPest】每日病虫害检测
// ============================================================================
void CropGrowthSystem::HandleDailyDiseaseAndPest(GameWorldState& world_state) {
    constexpr int kBaseInfectChance = 10;
    constexpr int kWateredInfectChance = 6;
    constexpr int kAutoClearChanceHighFavor = 30;
    constexpr int kAutoClearChanceDispatched = 70;

    auto& beast = world_state.GetSpiritBeast();
    const bool high_favor_beast = beast.favor >= 60;

    for (auto& plot : world_state.MutableTeaPlots()) {
        if (!plot.cleared || !plot.seeded || plot.ready) {
            continue;
        }

        const int infect_chance = plot.watered ? kWateredInfectChance : kBaseInfectChance;
        const bool has_issue_before = plot.disease || plot.pest;

        if (!plot.disease && (std::rand() % 100) < infect_chance) {
            plot.disease = true;
            if (hint_callback_) {
                hint_callback_("作物感染了病害！", 2.2f);
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
