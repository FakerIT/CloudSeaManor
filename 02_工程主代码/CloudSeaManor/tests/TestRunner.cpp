// CloudSeamanor Test Runner
// Executes all registered unit tests across domain and engine layers.

#include "TestFramework.hpp"

// 领域层测试
#include "SkillSystemTest.cpp"
#include "InventoryTest.cpp"
#include "StaminaTest.cpp"
#include "CropDataTest.cpp"
#include "CloudAndClockTest.cpp"

// 引擎层测试
#include "InputManagerTest.cpp"

int main() {
    return CloudSeamanor::engine::RunAllTests();
}
