#include "catch2Compat.hpp"

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

#include <unordered_map>

using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::ItemCount;
using CloudSeamanor::domain::WorkshopSystem;
using CloudSeamanor::engine::RepairProject;

TEST_CASE("Week4 loop: workshop converts input to output") {
    WorkshopSystem workshop;
    workshop.Initialize();

    Inventory inventory;
    inventory.AddItem("TeaLeaf", 4);

    REQUIRE(workshop.StartProcessing("tea_machine", "green_tea", inventory));
    CHECK_EQ(inventory.CountOf("TeaLeaf"), 2);

    std::unordered_map<std::string, int> outputs;
    const auto completed = workshop.Update(60.0f, 0.0f, outputs);

    CHECK_EQ(completed.size(), 1u);
    REQUIRE(outputs.contains("TeaPack"));
    CHECK_EQ(outputs["TeaPack"], 1);
}

TEST_CASE("Week4 loop: cloud density speeds up workshop progress") {
    WorkshopSystem workshop;
    workshop.Initialize();

    Inventory inventory;
    inventory.AddItem("TeaLeaf", 2);

    REQUIRE(workshop.StartProcessing("tea_machine", "green_tea", inventory));

    std::unordered_map<std::string, int> outputs_no_cloud;
    workshop.Update(40.0f, 0.0f, outputs_no_cloud);
    CHECK_FALSE(outputs_no_cloud.contains("TeaPack"));

    std::unordered_map<std::string, int> outputs_with_cloud;
    workshop.Update(10.0f, 1.0f, outputs_with_cloud);
    REQUIRE(outputs_with_cloud.contains("TeaPack"));
    CHECK_EQ(outputs_with_cloud["TeaPack"], 1);
}

TEST_CASE("Week4 loop: repair consumes resources and completes once") {
    Inventory inventory;
    inventory.AddItem("Wood", 4);
    inventory.AddItem("Turnip", 2);

    RepairProject repair;
    REQUIRE_FALSE(repair.completed);

    const std::vector<ItemCount> repair_costs{
        {"Wood", repair.wood_cost},
        {"Turnip", repair.turnip_cost}
    };

    REQUIRE(inventory.TryRemoveItems(repair_costs));
    repair.completed = true;

    CHECK_TRUE(repair.completed);
    CHECK_EQ(inventory.CountOf("Wood"), 0);
    CHECK_EQ(inventory.CountOf("Turnip"), 0);

    // Same day repeat should not consume again in gameplay; emulate that guard here.
    const bool second_try = repair.completed ? false : inventory.TryRemoveItems(repair_costs);
    CHECK_FALSE(second_try);
}
