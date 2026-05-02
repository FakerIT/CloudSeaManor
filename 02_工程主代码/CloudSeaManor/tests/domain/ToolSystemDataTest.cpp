#include "TestFramework.hpp"

#include "CloudSeamanor/domain/ToolSystem.hpp"

using namespace CloudSeamanor::domain;

TEST_CASE(ToolSystem_LoadsRuntimeToolDataFromCsv) {
    ToolSystem system;
    system.Initialize(ToolUpgradeConfig{});

    const ToolEffect effect = system.GetEffect(ToolType::Hoe);
    ASSERT_EQ(effect.display_name, std::string("普通锄头"));
    ASSERT_FLOAT_EQ(effect.efficiency_multiplier, 1.0f, 0.001f);

    system.SetLevel(ToolType::WateringCan, 5);
    const ToolEffect watering = system.GetEffect(ToolType::WateringCan);
    ASSERT_EQ(watering.display_name, std::string("灵金水壶"));
    ASSERT_EQ(watering.sprinkler_coverage, 4);
    ASSERT_FLOAT_EQ(watering.range_multiplier, 4.0f, 0.001f);
    return true;
}
