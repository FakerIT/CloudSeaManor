#include "catch2Compat.hpp"

#include "CloudSeamanor/CloudGuardianContract.hpp"

using CloudSeamanor::domain::AuraStage;
using CloudSeamanor::domain::CloudGuardianContract;

TEST_CASE("CloudGuardianContract::Initialize - creates six volumes") {
    CloudGuardianContract contract;
    contract.Initialize();

    CHECK_EQ(contract.Volumes().size(), 6u);
    CHECK_TRUE(contract.IsVolumeUnlocked(1));
    CHECK_FALSE(contract.IsVolumeUnlocked(2));
}

TEST_CASE("CloudGuardianContract::DeliverItem - completes unlocked matching pacts") {
    CloudGuardianContract contract;
    contract.Initialize();

    const bool completed_any = contract.DeliverItem("TeaLeaf", 1);

    CHECK_TRUE(completed_any);
    CHECK_EQ(contract.CompletedPactCount(), 4);
    CHECK_EQ(contract.CompletedVolumeCount(), 0);
}

TEST_CASE("CloudGuardianContract::CheckVolumeUnlocks - unlocks next volume") {
    CloudGuardianContract contract;
    contract.Initialize();
    contract.DeliverItem("TeaLeaf", 1);  // completes all pacts in volume 1

    contract.CheckVolumeUnlocks();

    CHECK_EQ(contract.CompletedVolumeCount(), 1);
    CHECK_TRUE(contract.IsVolumeUnlocked(2));
}

TEST_CASE("CloudGuardianContract::SetTrackingVolume - rejects locked volume") {
    CloudGuardianContract contract;
    contract.Initialize();
    contract.SetTrackingVolume(2);

    const auto* tracking = contract.GetTrackingVolume();
    REQUIRE(tracking != nullptr);
    CHECK_EQ(tracking->volume_id, 1);
}

TEST_CASE("CloudGuardianContract::SaveState/LoadState - preserves progress") {
    CloudGuardianContract original;
    original.Initialize();
    original.DeliverItem("TeaLeaf", 1);
    original.CheckVolumeUnlocks();
    original.SetTrackingVolume(2);

    const std::string saved = original.SaveState();

    CloudGuardianContract restored;
    restored.LoadState(saved);
    restored.SetTrackingVolume(2);

    CHECK_EQ(restored.CompletedVolumeCount(), 1);
    CHECK_EQ(restored.CompletedPactCount(), 4);
    CHECK_TRUE(restored.IsVolumeUnlocked(2));
    REQUIRE(restored.GetTrackingVolume() != nullptr);
    CHECK_EQ(restored.GetTrackingVolume()->volume_id, 2);
}

TEST_CASE("CloudGuardianContract::CurrentAuraStage - changes by completion ratio") {
    CloudGuardianContract contract;
    contract.Initialize();

    CHECK_EQ(contract.CurrentAuraStage(), AuraStage::Barren);

    contract.DeliverItem("TeaLeaf", 1);  // 4 / 19 > 20%
    CHECK_EQ(contract.CurrentAuraStage(), AuraStage::Awakened);
}
