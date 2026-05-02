#include "CloudSeamanor/engine/TeaGarden.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

void TeaGarden::UpdatePlots(
    std::vector<TeaPlot>& plots,
    float delta_seconds,
    CloudSeamanor::domain::CloudState cloud_state,
    int day_of_year) {
    constexpr int kWeeklyWaterWindowDays = 7;
    constexpr int kTeaGardenWaterGraceDays = 2;
    constexpr float kTideTeaQualityMultiplier = 1.2f;

    for (auto& plot : plots) {
        if (!plot.seeded || plot.ready) {
            continue;
        }

        const bool weekly_water_valid = plot.watered || ((day_of_year % kWeeklyWaterWindowDays) < kTeaGardenWaterGraceDays);
        if (!weekly_water_valid) {
            continue;
        }

        float growth_multiplier = 1.0f;
        if (cloud_state == CloudSeamanor::domain::CloudState::Tide) {
            growth_multiplier *= kTideTeaQualityMultiplier;
        }

        plot.growth += delta_seconds * growth_multiplier;
        if (plot.growth >= std::max(160.0f, plot.growth_time)) {
            plot.ready = true;
            plot.harvest_item_id = "TeaLeaf";
        }
    }
}

} // namespace CloudSeamanor::engine
