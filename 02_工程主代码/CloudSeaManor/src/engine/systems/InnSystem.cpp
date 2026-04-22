#include "CloudSeamanor/engine/systems/InnSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

void InnSystem::DailySettlement(GameWorldState& world_state, int house_level) {
    const int level = std::max(1, house_level);
    const int visitors = 2 + level * 2;
    const int avg_order_value = 18 + level * 6;
    const int income = visitors * avg_order_value;
    const int upkeep = 10 + level * 5;
    const int profit = std::max(0, income - upkeep);

    world_state.GetInnGoldReserve() += profit;
    world_state.GetGold() += profit;
}

} // namespace CloudSeamanor::engine
