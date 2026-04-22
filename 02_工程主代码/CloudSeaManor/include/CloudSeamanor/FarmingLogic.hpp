#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/GameClock.hpp"

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

[[nodiscard]] inline std::string SeasonTag(domain::Season season) {
    using domain::Season;
    switch (season) {
    case Season::Spring: return "spring";
    case Season::Summer: return "summer";
    case Season::Autumn: return "autumn";
    case Season::Winter: return "winter";
    }
    return "spring";
}

[[nodiscard]] inline bool ShouldPlotWiltBecauseSeason(const TeaPlot& plot, domain::Season current_season) {
    if (!plot.seeded) return false;
    if (plot.in_greenhouse) return false;
    if (plot.crop_id.empty()) return false;
    const std::string tag = SeasonTag(current_season);
    return plot.crop_id.find(tag) == std::string::npos;
}

inline void ApplySprinklerAutoWater(std::vector<TeaPlot>& plots, float radius) {
    const float r2 = std::max(0.0f, radius) * std::max(0.0f, radius);
    for (const auto& src : plots) {
        if (!src.sprinkler_installed || src.sprinkler_days_left <= 0) continue;
        const sf::Vector2f src_pos = src.shape.getPosition();
        for (auto& p : plots) {
            if (!p.cleared || !p.seeded || p.ready) continue;
            const sf::Vector2f pos = p.shape.getPosition();
            const sf::Vector2f d = pos - src_pos;
            if ((d.x * d.x + d.y * d.y) <= r2) {
                p.watered = true;
            }
        }
    }
}

} // namespace CloudSeamanor::engine

