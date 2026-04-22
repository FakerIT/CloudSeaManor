#include "catch2Compat.hpp"

#include "CloudSeamanor/InteractionSystem.hpp"

#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/PickupDrop.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

using CloudSeamanor::domain::Interactable;
using CloudSeamanor::domain::InteractableType;
using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::PickupDrop;
using CloudSeamanor::domain::SkillSystem;
using CloudSeamanor::domain::WorkshopSystem;
using CloudSeamanor::engine::InteractionCallbacks;
using CloudSeamanor::engine::InteractionResult;
using CloudSeamanor::engine::InteractionSystem;
using CloudSeamanor::engine::RepairProject;
using CloudSeamanor::engine::TeaMachine;
using CloudSeamanor::engine::TeaPlot;

namespace {

InteractionSystem MakeSystemWithHintCapture(std::string& out_hint) {
    InteractionSystem sys;
    InteractionCallbacks cb;
    cb.push_hint = [&](const std::string& msg, float) { out_hint = msg; };
    cb.log_info = [&](const std::string&) {};
    cb.update_hud = [&]() {};
    cb.refresh_window_title = [&]() {};
    cb.play_sfx = [&](const std::string&) {};
    cb.get_player_name = []() { return std::string("旅人"); };
    cb.get_farm_name = []() { return std::string("云海山庄"); };
    sys.Initialize(cb);
    return sys;
}

TeaPlot MakeBasicPlot() {
    TeaPlot plot;
    plot.crop_name = "萝卜";
    plot.seed_item_id = "TurnipSeed";
    plot.harvest_item_id = "Turnip";
    plot.harvest_amount = 2;
    plot.tilled = false;
    plot.seeded = false;
    plot.watered = false;
    plot.ready = false;
    plot.stage = 0;
    return plot;
}

Interactable MakeObject(InteractableType type, const std::string& label, const std::string& reward_item = "", int reward_amount = 0) {
    sf::RectangleShape shape;
    shape.setPosition({10.0f, 20.0f});
    shape.setSize({20.0f, 20.0f});
    return Interactable(shape.getPosition(), shape.getSize(), type, label, reward_item, reward_amount);
}

}  // namespace

TEST_CASE("InteractionSystem::InteractWithPlot - tills first") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    std::vector<TeaPlot> plots = {MakeBasicPlot()};
    Inventory inv;
    SkillSystem skills;

    const InteractionResult r = sys.InteractWithPlot(0, plots, inv, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_TRUE(plots[0].tilled);
    CHECK_THAT(hint, Contains("tilled"));
}

TEST_CASE("InteractionSystem::InteractWithPlot - planting fails without seed") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    std::vector<TeaPlot> plots = {MakeBasicPlot()};
    plots[0].tilled = true;
    Inventory inv;
    SkillSystem skills;

    const InteractionResult r = sys.InteractWithPlot(0, plots, inv, skills, 0.0f, false);

    CHECK_FALSE(r.success);
    CHECK_FALSE(plots[0].seeded);
    CHECK_THAT(hint, Contains("Missing seed"));
}

TEST_CASE("InteractionSystem::InteractWithPlot - planting consumes seed and sets stage") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    std::vector<TeaPlot> plots = {MakeBasicPlot()};
    plots[0].tilled = true;
    Inventory inv;
    inv.AddItem("TurnipSeed", 1);
    SkillSystem skills;

    const InteractionResult r = sys.InteractWithPlot(0, plots, inv, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_TRUE(plots[0].seeded);
    CHECK_EQ(plots[0].stage, 1);
    CHECK_EQ(inv.CountOf("TurnipSeed"), 0);
    CHECK_THAT(hint, Contains("planted"));
}

TEST_CASE("InteractionSystem::InteractWithPlot - watering sets watered") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    std::vector<TeaPlot> plots = {MakeBasicPlot()};
    plots[0].tilled = true;
    plots[0].seeded = true;
    Inventory inv;
    SkillSystem skills;

    const InteractionResult r = sys.InteractWithPlot(0, plots, inv, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_TRUE(plots[0].watered);
    CHECK_THAT(hint, Contains("watered"));
}

TEST_CASE("InteractionSystem::InteractWithObject - workstation starts tea machine when leaves enough") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    Inventory inv;
    inv.AddItem("TeaLeaf", 2);
    WorkshopSystem workshop;
    workshop.Initialize();
    RepairProject repair;
    std::vector<PickupDrop> pickups;
    std::vector<sf::RectangleShape> obstacles;
    SkillSystem skills;
    const std::vector<Interactable> objects = {MakeObject(InteractableType::Workstation, "Tea Machine")};

    const InteractionResult r = sys.InteractWithObject(
        0, objects, pickups, inv, &workshop, repair, obstacles, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_THAT(r.message, Contains("started"));
    CHECK_EQ(inv.CountOf("TeaLeaf"), 0);
}

TEST_CASE("InteractionSystem::InteractWithObject - workstation warns when leaves missing") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    Inventory inv;
    WorkshopSystem workshop;
    workshop.Initialize();
    RepairProject repair;
    std::vector<PickupDrop> pickups;
    std::vector<sf::RectangleShape> obstacles;
    SkillSystem skills;
    const std::vector<Interactable> objects = {MakeObject(InteractableType::Workstation, "Tea Machine")};

    const InteractionResult r = sys.InteractWithObject(
        0, objects, pickups, inv, &workshop, repair, obstacles, skills, 0.0f, false);

    CHECK_FALSE(r.success);
    CHECK_THAT(hint, Contains("Need Leaves"));
}

TEST_CASE("InteractionSystem::InteractWithObject - storage repairs when costs met") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    Inventory inv;
    inv.AddItem("Wood", 4);
    inv.AddItem("Turnip", 2);
    RepairProject repair;
    repair.completed = false;
    repair.wood_cost = 4;
    repair.turnip_cost = 2;

    WorkshopSystem workshop;
    workshop.Initialize();
    std::vector<PickupDrop> pickups;
    std::vector<sf::RectangleShape> obstacles(1);
    SkillSystem skills;
    const std::vector<Interactable> objects = {MakeObject(InteractableType::Storage, "Main House")};

    const InteractionResult r = sys.InteractWithObject(
        0, objects, pickups, inv, &workshop, repair, obstacles, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_TRUE(repair.completed);
    CHECK_THAT(hint, Contains("repaired"));
}

TEST_CASE("InteractionSystem::InteractWithObject - gathering node creates pickup") {
    std::string hint;
    auto sys = MakeSystemWithHintCapture(hint);
    Inventory inv;
    WorkshopSystem workshop;
    workshop.Initialize();
    RepairProject repair;
    std::vector<PickupDrop> pickups;
    std::vector<sf::RectangleShape> obstacles;
    SkillSystem skills;
    const std::vector<Interactable> objects = {MakeObject(InteractableType::GatheringNode, "草丛", "TeaLeaf", 2)};

    const InteractionResult r = sys.InteractWithObject(
        0, objects, pickups, inv, &workshop, repair, obstacles, skills, 0.0f, false);

    CHECK_TRUE(r.success);
    CHECK_EQ(pickups.size(), 1u);
    CHECK_THAT(hint, Contains("Gathered"));
}
