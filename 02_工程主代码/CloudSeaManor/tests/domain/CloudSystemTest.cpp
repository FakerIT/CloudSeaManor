#include "../catch2Compat.hpp"

#include "CloudSeamanor/CloudSystem.hpp"

using CloudSeamanor::domain::CloudState;
using CloudSeamanor::domain::CloudSystem;

TEST_CASE("CloudSystem::SetSpiritEnergy clamps to non-negative") {
    CloudSystem cloud;
    cloud.SetSpiritEnergy(-5);
    CHECK_EQ(cloud.SpiritEnergy(), 0);
}

TEST_CASE("CloudSystem::ForceTide sets both current and forecast tide") {
    CloudSystem cloud;
    cloud.ForceTide();

    CHECK_EQ(cloud.CurrentState(), CloudState::Tide);
    CHECK_EQ(cloud.ForecastState(), CloudState::Tide);
}

TEST_CASE("CloudSystem::CycleDebugState cycles clear->mist->dense->clear") {
    CloudSystem cloud;
    cloud.SetStates(CloudState::Clear, CloudState::Clear);
    cloud.CycleDebugState();
    CHECK_EQ(cloud.CurrentState(), CloudState::Mist);

    cloud.CycleDebugState();
    CHECK_EQ(cloud.CurrentState(), CloudState::DenseCloud);

    cloud.CycleDebugState();
    CHECK_EQ(cloud.CurrentState(), CloudState::Clear);
}

TEST_CASE("CloudSystem::UpdateForecastVisibility toggles after 22") {
    CloudSystem cloud;
    cloud.UpdateForecastVisibility(20, 21);
    CHECK_FALSE(cloud.IsForecastVisible());

    cloud.UpdateForecastVisibility(20, 22);
    CHECK_TRUE(cloud.IsForecastVisible());
}

TEST_CASE("CloudSystem::SpiritEnergyGain matches state table") {
    CloudSystem cloud;

    cloud.SetStates(CloudState::Clear, CloudState::Clear);
    CHECK_EQ(cloud.SpiritEnergyGain(), 5);

    cloud.SetStates(CloudState::Mist, CloudState::Clear);
    CHECK_EQ(cloud.SpiritEnergyGain(), 12);

    cloud.SetStates(CloudState::DenseCloud, CloudState::Clear);
    CHECK_EQ(cloud.SpiritEnergyGain(), 25);

    cloud.SetStates(CloudState::Tide, CloudState::Clear);
    CHECK_EQ(cloud.SpiritEnergyGain(), 80);
}

TEST_CASE("CloudSystem::SaveState and LoadState preserve values") {
    CloudSystem original;
    original.SetStates(CloudState::DenseCloud, CloudState::Tide);
    original.SetSpiritEnergy(321);
    original.ApplyPlayerInfluence(17);

    const std::string state = original.SaveState();

    CloudSystem restored;
    restored.LoadState(state);

    CHECK_EQ(restored.CurrentState(), CloudState::DenseCloud);
    CHECK_EQ(restored.ForecastState(), CloudState::Tide);
    CHECK_EQ(restored.SpiritEnergy(), 321);
    CHECK_EQ(restored.TotalPlayerInfluence(), 17);
}

TEST_CASE("CloudSystem::CurrentStateText and hint map correctly") {
    CloudSystem cloud;
    cloud.SetStates(CloudState::Mist, CloudState::Clear);

    CHECK_THAT(cloud.CurrentStateText(), Equals("薄雾"));
    CHECK_THAT(cloud.CurrentStateHint(), Contains("作物生长+15%"));
}
