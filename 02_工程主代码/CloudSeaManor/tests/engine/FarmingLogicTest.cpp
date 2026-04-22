#include "catch2Compat.hpp"

#include "CloudSeamanor/FarmingLogic.hpp"

using CloudSeamanor::engine::ApplySprinklerAutoWater;
using CloudSeamanor::engine::ShouldPlotWiltBecauseSeason;
using CloudSeamanor::engine::TeaPlot;
using CloudSeamanor::domain::Season;

TEST_CASE("FarmingLogic: greenhouse plot does not wilt across seasons") {
    TeaPlot plot;
    plot.seeded = true;
    plot.crop_id = "turnip_season_spring";
    plot.in_greenhouse = true;
    REQUIRE_FALSE(ShouldPlotWiltBecauseSeason(plot, Season::Summer));
}

TEST_CASE("FarmingLogic: non-greenhouse plot wilts when crop_id lacks season tag") {
    TeaPlot plot;
    plot.seeded = true;
    plot.crop_id = "turnip_season_spring";
    plot.in_greenhouse = false;
    REQUIRE(ShouldPlotWiltBecauseSeason(plot, Season::Summer));
}

TEST_CASE("FarmingLogic: sprinkler auto-water within radius") {
    TeaPlot sprinkler;
    sprinkler.shape.setPosition({0.0f, 0.0f});
    sprinkler.sprinkler_installed = true;
    sprinkler.sprinkler_days_left = 10;

    TeaPlot near_plot;
    near_plot.shape.setPosition({10.0f, 0.0f});
    near_plot.cleared = true;
    near_plot.seeded = true;
    near_plot.watered = false;

    TeaPlot far_plot;
    far_plot.shape.setPosition({200.0f, 0.0f});
    far_plot.cleared = true;
    far_plot.seeded = true;
    far_plot.watered = false;

    std::vector<TeaPlot> plots;
    plots.push_back(sprinkler);
    plots.push_back(near_plot);
    plots.push_back(far_plot);

    ApplySprinklerAutoWater(plots, 80.0f);

    CHECK(plots[1].watered);
    CHECK_FALSE(plots[2].watered);
}

TEST_CASE("FarmingLogic: sprinkler does nothing when days_left <= 0") {
    TeaPlot sprinkler;
    sprinkler.shape.setPosition({0.0f, 0.0f});
    sprinkler.sprinkler_installed = true;
    sprinkler.sprinkler_days_left = 0;

    TeaPlot near_plot;
    near_plot.shape.setPosition({10.0f, 0.0f});
    near_plot.cleared = true;
    near_plot.seeded = true;
    near_plot.watered = false;

    std::vector<TeaPlot> plots;
    plots.push_back(sprinkler);
    plots.push_back(near_plot);

    ApplySprinklerAutoWater(plots, 80.0f);

    CHECK_FALSE(plots[1].watered);
}

TEST_CASE("FarmingLogic: ShouldPlotWiltBecauseSeason returns false when crop_id empty") {
    TeaPlot plot;
    plot.seeded = true;
    plot.crop_id.clear();
    plot.in_greenhouse = false;
    REQUIRE_FALSE(ShouldPlotWiltBecauseSeason(plot, Season::Winter));
}

