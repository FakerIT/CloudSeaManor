#include "../catch2Compat.hpp"

#include "CloudSeamanor/WorkshopSystem.hpp"

using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::WorkshopSystem;

TEST_CASE("WorkshopSystem::Initialize - creates default machines") {
    WorkshopSystem workshop;
    workshop.Initialize();

    CHECK_EQ(workshop.GetMachines().size(), 2u);
    REQUIRE(workshop.GetMachine("tea_machine") != nullptr);
    REQUIRE(workshop.GetMachine("ferment_machine") != nullptr);
}

TEST_CASE("WorkshopSystem::StartProcessing - fails when recipe missing") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaLeaf", 10);

    const bool ok = workshop.StartProcessing("tea_machine", "missing_recipe", inv);
    CHECK_FALSE(ok);
    CHECK_EQ(inv.CountOf("TeaLeaf"), 10);
}

TEST_CASE("WorkshopSystem::StartProcessing - consumes inputs and starts machine") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaLeaf", 2);

    const bool ok = workshop.StartProcessing("tea_machine", "green_tea", inv);

    CHECK_TRUE(ok);
    CHECK_EQ(inv.CountOf("TeaLeaf"), 0);
    const auto* m = workshop.GetMachine("tea_machine");
    REQUIRE(m != nullptr);
    CHECK_TRUE(m->is_processing);
    CHECK_THAT(m->recipe_id, Equals("green_tea"));
}

TEST_CASE("WorkshopSystem::StartProcessing - respects unlocked slots") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaLeaf", 4);

    const bool first = workshop.StartProcessing("tea_machine", "green_tea", inv);
    const bool second = workshop.StartProcessing("ferment_machine", "tea_ferment", inv);

    CHECK_TRUE(first);
    CHECK_FALSE(second);
}

TEST_CASE("WorkshopSystem::StartProcessing - respects machine required level") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaPack", 2);

    const bool ok = workshop.StartProcessing("ferment_machine", "tea_ferment", inv);
    CHECK_FALSE(ok);
}

TEST_CASE("WorkshopSystem::Upgrade - increases level and slots") {
    WorkshopSystem workshop;
    workshop.Initialize();

    CHECK_TRUE(workshop.Upgrade(2, 2));
    CHECK_EQ(workshop.Level(), 2);
    CHECK_EQ(workshop.UnlockedSlots(), 2);
    CHECK_FALSE(workshop.Upgrade(2, 2));
}

TEST_CASE("WorkshopSystem::Update - produces outputs and completes machine") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaLeaf", 2);
    REQUIRE(workshop.StartProcessing("tea_machine", "green_tea", inv));

    std::unordered_map<std::string, int> outputs;
    const auto completed = workshop.Update(60.0f, 0.0f, outputs);

    CHECK_EQ(completed.size(), 1u);
    CHECK_THAT(completed[0], Equals("tea_machine"));
    CHECK_EQ(outputs["TeaPack"], 1);

    const auto* m = workshop.GetMachine("tea_machine");
    REQUIRE(m != nullptr);
    CHECK_FALSE(m->is_processing);
    CHECK_THAT(m->recipe_id, Equals(""));
}

TEST_CASE("WorkshopSystem::Update - cloud density speeds up progress") {
    WorkshopSystem workshop;
    workshop.Initialize();
    Inventory inv;
    inv.AddItem("TeaLeaf", 2);
    REQUIRE(workshop.StartProcessing("tea_machine", "green_tea", inv));

    std::unordered_map<std::string, int> outputs;
    workshop.Update(30.0f, 0.0f, outputs);
    const float p0 = workshop.GetMachineProgress("tea_machine");

    workshop.SetMachineState("tea_machine", "green_tea", 0.0f, true);
    outputs.clear();
    workshop.Update(30.0f, 1.0f, outputs);
    const float p1 = workshop.GetMachineProgress("tea_machine");

    CHECK_TRUE(p1 > p0);
}

TEST_CASE("WorkshopSystem::SetMachineState clamps progress range") {
    WorkshopSystem workshop;
    workshop.Initialize();

    CHECK_TRUE(workshop.SetMachineState("tea_machine", "green_tea", 150.0f, true));
    CHECK_LE(workshop.GetMachineProgress("tea_machine"), 100.0f);
    CHECK_TRUE(workshop.SetMachineState("tea_machine", "green_tea", -50.0f, true));
    CHECK_LE(0.0f, workshop.GetMachineProgress("tea_machine"));
}
