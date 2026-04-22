#include "catch2Compat.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"

using CloudSeamanor::engine::CropGrowthSystem;
using CloudSeamanor::engine::GameWorldState;
using CloudSeamanor::engine::TeaPlot;

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

