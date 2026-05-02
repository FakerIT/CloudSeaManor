#include "TestFramework.hpp"

#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"

#include <filesystem>

using namespace CloudSeamanor::domain;

namespace {

std::filesystem::path DynamicLifeTestSourceRoot() {
    return std::filesystem::weakly_canonical(
        std::filesystem::current_path().parent_path().parent_path());
}

}

TEST_CASE(DynamicLifeSystem_LoadsNpcProfilesFromCsv) {
    DynamicLifeSystem system;
    ASSERT_TRUE(system.LoadFromFile((DynamicLifeTestSourceRoot() / "assets/data/npc/npc_data.csv").string()));

    const NpcLifeState* lin = system.GetNpcState("lin");
    const NpcLifeState* acha = system.GetNpcState("acha");
    ASSERT_TRUE(lin != nullptr);
    ASSERT_TRUE(acha != nullptr);
    ASSERT_EQ(system.GetNpcStageText("lin"), std::string("初遇期 (0点)"));

    system.ApplyStageTransition("lin", LifeStage::Stage1);
    ASSERT_TRUE(static_cast<int>(lin->stage) >= 1);
    return true;
}
