#include "catch2Compat.hpp"

#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/GameAppRuntimeTypes.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

using CloudSeamanor::domain::CloudSystem;
using CloudSeamanor::domain::DynamicLifeSystem;
using CloudSeamanor::domain::GameClock;
using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::SkillSystem;
using CloudSeamanor::domain::StaminaSystem;
using CloudSeamanor::domain::WorkshopSystem;
using CloudSeamanor::engine::InteractionState;
using CloudSeamanor::engine::MailOrderEntry;
using CloudSeamanor::engine::NpcActor;
using CloudSeamanor::engine::PlayerInteractRuntimeContext;
using CloudSeamanor::engine::PriceTableEntry;
using CloudSeamanor::engine::RepairProject;
using CloudSeamanor::engine::SpiritBeast;
using CloudSeamanor::engine::TeaMachine;
using CloudSeamanor::engine::TeaPlot;

TEST_CASE("PlayerInteractRuntime: uncleared plot requires wood and clears state") {
    GameClock clock;
    CloudSystem cloud;
    Inventory inv;
    StaminaSystem stamina;
    SkillSystem skills;
    DynamicLifeSystem life;
    WorkshopSystem workshop;
    workshop.Initialize();

    std::vector<TeaPlot> plots;
    TeaPlot p;
    p.cleared = false;
    p.tilled = true;
    p.seeded = true;
    p.watered = true;
    p.ready = true;
    p.growth = 123.0f;
    p.stage = 4;
    plots.push_back(p);

    std::vector<NpcActor> npcs;
    SpiritBeast beast;
    bool beast_watered_today = false;
    std::vector<CloudSeamanor::engine::HeartParticle> hearts;
    std::vector<CloudSeamanor::domain::PickupDrop> pickups;
    std::vector<CloudSeamanor::domain::Interactable> interactables;
    RepairProject repair;
    TeaMachine machine;
    std::vector<sf::RectangleShape> obstacles;
    CloudSeamanor::engine::NpcTextMappings mappings;
    CloudSeamanor::engine::DialogueEngine engine;
    std::string dialogue_text;
    int gold = 0;
    std::vector<PriceTableEntry> price_table;
    std::vector<MailOrderEntry> mails;
    CloudSeamanor::domain::CropQuality last_trade_q = CloudSeamanor::domain::CropQuality::Normal;
    bool in_spirit = false;
    std::unordered_map<std::string, int> plant_cd;
    std::unordered_map<std::string, int> weekly_buy;
    std::unordered_map<std::string, int> weekly_sell;
    std::vector<std::string> daily_stock;
    int inn_reserve = 0;
    int coop_fed = 0;
    int deco_score = 0;
    std::string pet_type;
    bool pet_adopted = false;
    std::unordered_map<std::string, bool> achievements;
    std::vector<std::string> mod_hooks;
    InteractionState interaction_state;
    int highlighted_npc_index = -1;
    int highlighted_plot_index = 0;
    int highlighted_index = -1;
    bool spirit_beast_highlighted = false;

    bool refreshed = false;
    auto refresh_plot = [&](TeaPlot& /*plot*/, bool /*hl*/) { refreshed = true; };

    PlayerInteractRuntimeContext ctx(
        clock,
        cloud,
        inv,
        stamina,
        0.0f,
        plots,
        npcs,
        beast,
        beast_watered_today,
        hearts,
        pickups,
        interactables,
        repair,
        machine,
        obstacles,
        skills,
        life,
        workshop,
        mappings,
        engine,
        "assets/data",
        dialogue_text,
        gold,
        price_table,
        mails,
        last_trade_q,
        in_spirit,
        plant_cd,
        weekly_buy,
        weekly_sell,
        daily_stock,
        inn_reserve,
        coop_fed,
        deco_score,
        pet_type,
        pet_adopted,
        achievements,
        mod_hooks,
        0,
        [](const std::string&, float) {},
        nullptr,
        []() {},
        []() {},
        []() {},
        [](CloudSeamanor::domain::SkillType, int) {},
        refresh_plot,
        [](const sf::Vector2f&, std::vector<CloudSeamanor::engine::HeartParticle>&) {},
        [](CloudSeamanor::domain::PickupDrop&) {},
        interaction_state,
        highlighted_npc_index, highlighted_plot_index, highlighted_index,
        1,
        8,
        spirit_beast_highlighted);

    // Not enough wood
    REQUIRE(CloudSeamanor::engine::HandlePrimaryInteraction(ctx));
    CHECK_FALSE(ctx.tea_plots[0].cleared);

    // Enough wood
    ctx.inventory.AddItem("Wood", 5);
    refreshed = false;
    REQUIRE(CloudSeamanor::engine::HandlePrimaryInteraction(ctx));
    CHECK(ctx.tea_plots[0].cleared);
    CHECK_FALSE(ctx.tea_plots[0].tilled);
    CHECK_FALSE(ctx.tea_plots[0].seeded);
    CHECK_FALSE(ctx.tea_plots[0].watered);
    CHECK_FALSE(ctx.tea_plots[0].ready);
    CHECK_EQ(static_cast<int>(ctx.tea_plots[0].growth), 0);
    CHECK_EQ(ctx.tea_plots[0].stage, 0);
    CHECK(refreshed);
}

