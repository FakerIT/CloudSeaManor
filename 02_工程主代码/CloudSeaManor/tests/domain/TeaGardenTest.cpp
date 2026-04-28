#include "../catch2Compat.hpp"

#include "CloudSeamanor/TeaGarden.hpp"

using CloudSeamanor::domain::CloudState;
using CloudSeamanor::domain::CropQuality;
using CloudSeamanor::domain::TeaGarden;
using CloudSeamanor::engine::TeaPlot;

namespace {

TeaPlot MakeSeededPlot() {
    TeaPlot plot;
    plot.seeded = true;
    plot.ready = false;
    plot.watered = true;
    plot.growth = 0.0f;
    plot.growth_time = 160.0f;
    plot.quality = CropQuality::Normal;
    return plot;
}

}  // namespace

TEST_CASE("TeaGarden::UpdatePlots - ignores unseeded plot") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1);
    plots[0].seeded = false;
    plots[0].growth = 42.0f;

    garden.UpdatePlots(plots, 20.0f, CloudState::Clear, 1);

    CHECK_FLOAT_EQ(plots[0].growth, 42.0f, 0.001f);
}

TEST_CASE("TeaGarden::UpdatePlots - ignores ready plot") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1, MakeSeededPlot());
    plots[0].ready = true;
    plots[0].growth = 120.0f;

    garden.UpdatePlots(plots, 30.0f, CloudState::Clear, 1);

    CHECK_FLOAT_EQ(plots[0].growth, 120.0f, 0.001f);
}

TEST_CASE("TeaGarden::UpdatePlots - skips when outside weekly water window") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1, MakeSeededPlot());
    plots[0].watered = false;

    garden.UpdatePlots(plots, 20.0f, CloudState::Clear, 3);  // 3 % 7 = 3, not in grace [0,1]

    CHECK_FLOAT_EQ(plots[0].growth, 0.0f, 0.001f);
}

TEST_CASE("TeaGarden::UpdatePlots - allows growth in weekly grace window") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1, MakeSeededPlot());
    plots[0].watered = false;

    garden.UpdatePlots(plots, 20.0f, CloudState::Clear, 8);  // 8 % 7 = 1, in grace window

    CHECK_FLOAT_EQ(plots[0].growth, 20.0f, 0.001f);
}

TEST_CASE("TeaGarden::UpdatePlots - tide boosts growth and upgrades quality") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1, MakeSeededPlot());

    garden.UpdatePlots(plots, 100.0f, CloudState::Tide, 1);

    CHECK_FLOAT_EQ(plots[0].growth, 120.0f, 0.001f);
    CHECK_EQ(plots[0].quality, CropQuality::Spirit);
}

TEST_CASE("TeaGarden::UpdatePlots - marks ready and sets harvest item") {
    TeaGarden garden;
    std::vector<TeaPlot> plots(1, MakeSeededPlot());

    garden.UpdatePlots(plots, 150.0f, CloudState::Tide, 1);  // 150 * 1.2 = 180 >= 160

    CHECK_TRUE(plots[0].ready);
    CHECK_THAT(plots[0].harvest_item_id, Equals("TeaLeaf"));
}
