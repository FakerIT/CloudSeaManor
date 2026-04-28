#include "CloudSeamanor/GameAppWeather.hpp"

namespace CloudSeamanor::engine {

void UpdateWeatherState(
    CloudSeamanor::domain::CloudSystem& cloud_system,
    const CloudSeamanor::domain::GameClock& clock,
    int seeded_plots_count,
    bool main_house_repaired,
    bool spirit_beast_interacted
) {
    cloud_system.AdvanceToNextDay(clock.Day());
    int daily_influence = 0;
    daily_influence += seeded_plots_count * 5;
    if (!main_house_repaired) daily_influence += 5;
    if (!spirit_beast_interacted) daily_influence -= 3;
    cloud_system.ApplyPlayerInfluence(daily_influence);
    cloud_system.UpdateForecastVisibility(clock.Day(), clock.Hour());
}

} // namespace CloudSeamanor::engine
