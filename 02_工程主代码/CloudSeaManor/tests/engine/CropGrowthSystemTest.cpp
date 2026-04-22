#include "catch2Compat.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"

using CloudSeamanor::engine::CropGrowthSystem;
using CloudSeamanor::engine::GameWorldState;
using CloudSeamanor::engine::TeaPlot;
using CloudSeamanor::domain::Season;

TEST_CASE("CropGrowthSystem: growth uses seconds and reaches ready at growth_time") {
    GameWorldState state;
    state.GetTeaPlots().clear();

    TeaPlot plot;
    plot.cleared = true;
    plot.seeded = true;
    plot.watered = true;
    plot.ready = false;
    plot.growth = 0.0f;
    plot.stage = 0;
    plot.growth_time = 10.0f;
    plot.growth_stages = 4;

    state.GetTeaPlots().push_back(plot);

    CropGrowthSystem sys;
    sys.Update(state, 5.0f);
    CHECK_FALSE(state.GetTeaPlots()[0].ready);
    CHECK(state.GetTeaPlots()[0].growth >= 5.0f);
    CHECK(state.GetTeaPlots()[0].stage >= 2);

    sys.Update(state, 5.0f);
    CHECK(state.GetTeaPlots()[0].ready);
    CHECK(state.GetTeaPlots()[0].stage >= state.GetTeaPlots()[0].growth_stages);
}

TEST_CASE("CropGrowthSystem: fertilizer multipliers affect growth") {
    GameWorldState state;
    state.GetTeaPlots().clear();

    TeaPlot plot;
    plot.cleared = true;
    plot.seeded = true;
    plot.watered = true;
    plot.ready = false;
    plot.growth = 0.0f;
    plot.stage = 0;
    plot.growth_time = 100.0f;
    plot.growth_stages = 4;
    plot.fertilized = true;
    plot.fertilizer_type = "basic";
    state.GetTeaPlots().push_back(plot);

    CropGrowthSystem sys;
    sys.SetFertilizerMultipliers(2.0f, 3.0f);
    sys.Update(state, 10.0f);
    const float g_basic = state.GetTeaPlots()[0].growth;

    state.GetTeaPlots()[0].growth = 0.0f;
    state.GetTeaPlots()[0].stage = 0;
    state.GetTeaPlots()[0].fertilizer_type = "premium";
    sys.Update(state, 10.0f);
    const float g_premium = state.GetTeaPlots()[0].growth;

    CHECK_TRUE(g_premium > g_basic);
}

TEST_CASE("CropGrowthSystem: greenhouse plots grow faster even without fertilizer") {
    GameWorldState state;
    state.GetTeaPlots().clear();

    TeaPlot normal;
    normal.cleared = true;
    normal.seeded = true;
    normal.watered = true;
    normal.ready = false;
    normal.growth_time = 100.0f;
    normal.growth_stages = 4;
    normal.in_greenhouse = false;

    TeaPlot greenhouse = normal;
    greenhouse.in_greenhouse = true;

    state.GetTeaPlots().push_back(normal);
    state.GetTeaPlots().push_back(greenhouse);

    CropGrowthSystem sys;
    sys.Update(state, 10.0f);

    CHECK_TRUE(state.GetTeaPlots()[1].growth > state.GetTeaPlots()[0].growth);
}

TEST_CASE("CropGrowthSystem: sprinkler allows growth without manual watering") {
    GameWorldState state;
    state.GetTeaPlots().clear();

    TeaPlot plot;
    plot.cleared = true;
    plot.seeded = true;
    plot.watered = false;
    plot.sprinkler_installed = true;
    plot.sprinkler_days_left = 3;
    plot.ready = false;
    plot.growth_time = 20.0f;
    plot.growth_stages = 4;
    state.GetTeaPlots().push_back(plot);

    CropGrowthSystem sys;
    sys.Update(state, 5.0f);
    CHECK_TRUE(state.GetTeaPlots()[0].growth > 0.0f);
}

TEST_CASE("CropGrowthSystem: HandleSeasonChanged wilts non-greenhouse mismatched season crops") {
    GameWorldState state;
    state.GetTeaPlots().clear();

    TeaPlot plot;
    plot.cleared = true;
    plot.seeded = true;
    plot.watered = true;
    plot.ready = false;
    plot.growth = 50.0f;
    plot.stage = 2;
    plot.crop_id = "turnip_season_spring";
    plot.crop_name = "萝卜";
    plot.in_greenhouse = false;
    state.GetTeaPlots().push_back(plot);

    CropGrowthSystem sys;
    sys.HandleSeasonChanged(state, Season::Summer);

    CHECK_FALSE(state.GetTeaPlots()[0].seeded);
    CHECK_FALSE(state.GetTeaPlots()[0].watered);
    CHECK_FALSE(state.GetTeaPlots()[0].ready);
    CHECK_EQ(state.GetTeaPlots()[0].stage, 0);
    CHECK_FLOAT_EQ(state.GetTeaPlots()[0].growth, 0.0f, 0.001f);
}

