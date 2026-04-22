#include "CloudSeamanor/engine/systems/CoopSystem.hpp"

namespace CloudSeamanor::engine {

void CoopSystem::DailyUpdate(GameWorldState& world_state) {
    const int feed_bonus = world_state.GetCoopFedToday();
    if (feed_bonus > 0) {
        world_state.GetInventory().AddItem("Egg", feed_bonus);
    }
    world_state.GetCoopFedToday() = 0;
}

void BarnSystem::DailyUpdate(GameWorldState& world_state) {
    const int score = world_state.GetDecorationScore();
    const int milk_count = (score >= 20) ? 2 : 1;
    world_state.GetInventory().AddItem("Milk", milk_count);
}

} // namespace CloudSeamanor::engine
