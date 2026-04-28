// CloudSeamanor Test Runner
// Executes all registered unit tests across domain and engine layers.

#include "TestFramework.hpp"

// 说明：本仓库历史上同时存在两套测试风格（TestFramework / Catch2Compat）。
// 本轮先统一为 TestFramework，让测试目标可编译可运行；其他测试文件后续再逐步迁移。

// 引擎层（旧测试框架）
#include "InputManagerTest.cpp"
#include "engine/AudioManagerConfigTest.cpp"
#include "engine/BattleBalanceSimulationTest.cpp"
#include "engine/DayCycleSingleEntryTest.cpp"
#include "engine/MailRulePersistenceTest.cpp"
#include "engine/NpcDeliverySystemTest.cpp"
#include "engine/GameAppNpcDataTest.cpp"
#include "engine/PickupSystemRuntimeTest.cpp"
#include "engine/RetentionLoopRegressionTest.cpp"
#include "engine/SystemExpansionRegressionTest.cpp"
#include "engine/UiLayoutSystemTest.cpp"

// 基础设施层测试
#include "infrastructure/GameConfigTest.cpp"
#include "infrastructure/DataRegistryTest.cpp"
#include "infrastructure/SaveSlotManagerTest.cpp"
#include "infrastructure/SaveGameStateTest.cpp"

// 领域层数据驱动测试
#include "domain/DiarySystemDataTest.cpp"
#include "domain/DynamicLifeSystemDataTest.cpp"
#include "domain/ToolSystemDataTest.cpp"

int main() {
    return CloudSeamanor::engine::RunAllTests();
}
