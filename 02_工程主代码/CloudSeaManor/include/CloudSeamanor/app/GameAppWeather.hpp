#pragma once

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"

namespace CloudSeamanor::engine {

void UpdateWeatherState(
    CloudSeamanor::domain::CloudSystem& cloud_system,
    const CloudSeamanor::domain::GameClock& clock,
    int seeded_plots_count,
    bool main_house_repaired,
    bool spirit_beast_interacted
);

} // namespace CloudSeamanor::engine
