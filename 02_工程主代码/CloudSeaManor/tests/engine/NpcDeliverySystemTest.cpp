#include "../TestFramework.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/systems/NpcDeliverySystem.hpp"

using CloudSeamanor::engine::GameWorldState;
using CloudSeamanor::engine::NpcActor;
using CloudSeamanor::engine::NpcDeliverySystem;
using CloudSeamanor::engine::QuestState;
using CloudSeamanor::engine::RuntimeQuest;

TEST_CASE(NpcDeliverySystem_claim_rewards_updates_world_state) {
    GameWorldState world_state;
    NpcDeliverySystem delivery_system;

    NpcActor npc;
    npc.id = "lin";
    npc.favor = 10;
    world_state.MutableNpcs().push_back(npc);

    world_state.MutableInventory().AddItem("Wood", 2);
    world_state.SetGold(100);

    RuntimeQuest quest;
    quest.id = "npc_delivery|lin|Wood|2|30|5";
    quest.state = QuestState::Completed;
    world_state.MutableRuntimeQuests().push_back(quest);

    const bool claimed = delivery_system.TryClaimRewards(world_state, "lin");

    ASSERT_TRUE(claimed);
    ASSERT_EQ(world_state.GetGold(), 130);
    ASSERT_EQ(world_state.GetInventory().CountOf("Wood"), 0);
    ASSERT_EQ(world_state.MutableRuntimeQuests().front().state, QuestState::Claimed);
    ASSERT_EQ(world_state.GetNpcs().front().favor, 15);
    return true;
}
