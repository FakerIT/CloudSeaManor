#include "../TestFramework.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"

using CloudSeamanor::engine::GameWorldState;
using CloudSeamanor::engine::PickupSystem;
using CloudSeamanor::engine::PickupSystemRuntime;

TEST_CASE(PickupSystemRuntime_collect_nearby_moves_items_into_inventory) {
    GameWorldState world_state;
    PickupSystem pickup_system;

    int hint_count = 0;
    PickupSystemRuntime runtime(
        pickup_system,
        world_state,
        [&](const std::string&, float) {
            ++hint_count;
        });

    world_state.MutablePickups().emplace_back(sf::Vector2f(620.0f, 340.0f), "Wood", 2);
    ASSERT_EQ(static_cast<int>(world_state.MutablePickups().size()), 1);
    ASSERT_EQ(world_state.GetInventory().CountOf("Wood"), 0);

    runtime.CollectNearby();

    ASSERT_EQ(static_cast<int>(world_state.MutablePickups().size()), 0);
    ASSERT_EQ(world_state.GetInventory().CountOf("Wood"), 2);
    ASSERT_EQ(hint_count, 1);
    return true;
}
