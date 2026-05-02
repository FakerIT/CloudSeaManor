#include "CloudSeamanor/engine/systems/CoopSystem.hpp"

namespace CloudSeamanor::engine {

void CoopSystem::DailyUpdate(GameWorldState& world_state) {
    const int feed_bonus = world_state.GetCoopFedToday();
    const int decor = world_state.GetDecorationScore();
    int eggs_today = 0;
    if (feed_bonus > 0) {
        const int decor_bonus = std::clamp(decor / 30, 0, 1); // +1 egg at 30+
        eggs_today = std::min(4, 1 + feed_bonus + decor_bonus);
        world_state.MutableInventory().AddItem("Egg", eggs_today);
    }
    world_state.MutableLivestockEggsToday() = eggs_today;
}

void BarnSystem::DailyUpdate(GameWorldState& world_state) {
    const int score = world_state.GetDecorationScore();
    const int feed_bonus = world_state.GetCoopFedToday();
    int milk_today = 0;
    if (feed_bonus > 0) {
        // Deco tier bonus: +1 at 20+, +1 extra at 45+ (cap 3)
        milk_today = 1 + (score >= 20 ? 1 : 0) + (score >= 45 ? 1 : 0);
        milk_today = std::min(3, milk_today);
        world_state.MutableInventory().AddItem("Milk", milk_today);
    }
    world_state.MutableLivestockMilkToday() = milk_today;
    world_state.MutableCoopFedToday() = 0;
}

} // namespace CloudSeamanor::engine
