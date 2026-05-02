// ============================================================================
// 【StaminaTest.cpp】Stamina 单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::StaminaSystem
//
// Coverage:
// - Initialization
// - Consume operations
// - Recover operations
// - Protection period modifier
// - CanAfford checks
// - Ratio calculations
// - Boundary clamping (cannot go below 0 or above max)
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"

using CloudSeamanor::domain::StaminaSystem;

namespace {

StaminaSystem MakeStamina(float current, float max) {
    StaminaSystem s;
    s.Initialize(current, max);
    return s;
}

}  // namespace

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("StaminaSystem::Initialize - sets current and max values")
{
    StaminaSystem stamina;
    stamina.Initialize(80.0f, 100.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 80.0f, 0.001f);
    CHECK_FLOAT_EQ(stamina.Max(), 100.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Initialize - ratio is 1.0 when full")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f);

    CHECK_FLOAT_EQ(stamina.Ratio(), 1.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Initialize - ratio is 0.5 when half")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    CHECK_FLOAT_EQ(stamina.Ratio(), 0.5f, 0.001f);
}

TEST_CASE("StaminaSystem::Initialize - default protection values")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);

    CHECK_TRUE(stamina.IsInProtectionPeriod());
}

// ============================================================================
// Consume Tests
// ============================================================================

TEST_CASE("StaminaSystem::Consume - basic consume")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f);

    stamina.Consume(30.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 70.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Consume - cannot go below zero")
{
    StaminaSystem stamina;
    stamina.Initialize(10.0f, 100.0f);

    stamina.Consume(50.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Consume - negative amount is no-op")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.Consume(-10.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Consume - zero amount is no-op")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.Consume(0.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Consume - exact depletion")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.Consume(50.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
}

// ============================================================================
// ConsumeRaw Tests (不受保护期影响)
// ============================================================================

TEST_CASE("StaminaSystem::ConsumeRaw - bypasses protection")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);
    stamina.SetGameDay(1);

    stamina.ConsumeRaw(50.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
}

// ============================================================================
// Recover Tests
// ============================================================================

TEST_CASE("StaminaSystem::Recover - basic recover")
{
    StaminaSystem stamina;
    stamina.Initialize(30.0f, 100.0f);

    stamina.Recover(20.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Recover - cannot exceed max")
{
    StaminaSystem stamina;
    stamina.Initialize(90.0f, 100.0f);

    stamina.Recover(50.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Recover - negative amount is no-op")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.Recover(-10.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Recover - exact fill")
{
    StaminaSystem stamina;
    stamina.Initialize(20.0f, 100.0f);

    stamina.Recover(80.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
}

TEST_CASE("StaminaSystem::RecoverPerSecond - time-based recovery")
{
    StaminaSystem stamina;
    stamina.Initialize(0.0f, 100.0f);

    stamina.RecoverPerSecond(10.0f, 3.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 30.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Refill - restores to max")
{
    StaminaSystem stamina;
    stamina.Initialize(25.0f, 100.0f);

    stamina.Refill();

    CHECK_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
}

// ============================================================================
// SetMax / SetCurrent Tests
// ============================================================================

TEST_CASE("StaminaSystem::SetMax - updates max value")
{
    StaminaSystem stamina;
    stamina.Initialize(80.0f, 100.0f);

    stamina.SetMax(200.0f);

    CHECK_FLOAT_EQ(stamina.Max(), 200.0f, 0.001f);
}

TEST_CASE("StaminaSystem::SetMax - clamps current if exceeded")
{
    StaminaSystem stamina;
    stamina.Initialize(80.0f, 100.0f);

    stamina.SetMax(50.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
}

TEST_CASE("StaminaSystem::SetCurrent - updates current value")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.SetCurrent(75.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 75.0f, 0.001f);
}

TEST_CASE("StaminaSystem::SetCurrent - clamps to max")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.SetCurrent(150.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
}

TEST_CASE("StaminaSystem::SetCurrent - clamps to zero")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    stamina.SetCurrent(-20.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
}

// ============================================================================
// Protection Period Tests
// ============================================================================

TEST_CASE("StaminaSystem::IsInProtectionPeriod - within protection")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);

    stamina.SetGameDay(7);

    CHECK_TRUE(stamina.IsInProtectionPeriod());
}

TEST_CASE("StaminaSystem::IsInProtectionPeriod - at boundary")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);

    stamina.SetGameDay(14);

    CHECK_TRUE(stamina.IsInProtectionPeriod());
}

TEST_CASE("StaminaSystem::IsInProtectionPeriod - beyond protection")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);

    stamina.SetGameDay(15);

    CHECK_FALSE(stamina.IsInProtectionPeriod());
}

TEST_CASE("StaminaSystem::Consume - applies protection modifier")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);
    stamina.SetGameDay(5);

    stamina.Consume(100.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 30.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Consume - no modifier after protection")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);
    stamina.SetGameDay(20);

    stamina.Consume(100.0f);

    CHECK_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
}

TEST_CASE("StaminaSystem::GetConsumptionModifier - returns 0.7 in protection")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);
    stamina.SetGameDay(10);

    CHECK_FLOAT_EQ(stamina.GetConsumptionModifier(), 0.7f, 0.001f);
}

TEST_CASE("StaminaSystem::GetConsumptionModifier - returns 1.0 after protection")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f, 14, 0.7f);
    stamina.SetGameDay(30);

    CHECK_FLOAT_EQ(stamina.GetConsumptionModifier(), 1.0f, 0.001f);
}

// ============================================================================
// CanAfford Tests
// ============================================================================

TEST_CASE("StaminaSystem::CanAfford - sufficient stamina")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    CHECK_TRUE(stamina.CanAfford(30.0f));
}

TEST_CASE("StaminaSystem::CanAfford - exact match")
{
    StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);

    CHECK_TRUE(stamina.CanAfford(50.0f));
}

TEST_CASE("StaminaSystem::CanAfford - insufficient stamina")
{
    StaminaSystem stamina;
    stamina.Initialize(30.0f, 100.0f);

    CHECK_FALSE(stamina.CanAfford(50.0f));
}

TEST_CASE("StaminaSystem::CanAfford - zero cost always true")
{
    StaminaSystem stamina;
    stamina.Initialize(0.0f, 100.0f);

    CHECK_TRUE(stamina.CanAfford(0.0f));
}

TEST_CASE("StaminaSystem::CanAfford - negative cost always true")
{
    StaminaSystem stamina;
    stamina.Initialize(30.0f, 100.0f);

    CHECK_TRUE(stamina.CanAfford(-10.0f));
}

// ============================================================================
// Ratio Tests
// ============================================================================

TEST_CASE("StaminaSystem::Ratio - boundary 0")
{
    StaminaSystem stamina;
    stamina.Initialize(0.0f, 200.0f);

    CHECK_FLOAT_EQ(stamina.Ratio(), 0.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Ratio - boundary 1")
{
    StaminaSystem stamina;
    stamina.Initialize(200.0f, 200.0f);

    CHECK_FLOAT_EQ(stamina.Ratio(), 1.0f, 0.001f);
}

TEST_CASE("StaminaSystem::Ratio - zero max returns 0")
{
    StaminaSystem stamina;
    stamina.Initialize(0.0f, 0.0f);

    CHECK_FLOAT_EQ(stamina.Ratio(), 0.0f, 0.001f);
}

// ============================================================================
// Status Flag Tests
// ============================================================================

TEST_CASE("StaminaSystem::IsLow - below 20 percent")
{
    StaminaSystem stamina;
    stamina.Initialize(19.0f, 100.0f);

    CHECK_TRUE(stamina.IsLow());
}

TEST_CASE("StaminaSystem::IsLow - at 20 percent")
{
    StaminaSystem stamina;
    stamina.Initialize(20.0f, 100.0f);

    CHECK_FALSE(stamina.IsLow());
}

TEST_CASE("StaminaSystem::IsCritical - below 10 percent")
{
    StaminaSystem stamina;
    stamina.Initialize(9.0f, 100.0f);

    CHECK_TRUE(stamina.IsCritical());
}

TEST_CASE("StaminaSystem::IsCritical - at 10 percent")
{
    StaminaSystem stamina;
    stamina.Initialize(10.0f, 100.0f);

    CHECK_FALSE(stamina.IsCritical());
}

TEST_CASE("StaminaSystem::IsFull - when at max")
{
    StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f);

    CHECK_TRUE(stamina.IsFull());
}

TEST_CASE("StaminaSystem::IsFull - when below max")
{
    StaminaSystem stamina;
    stamina.Initialize(99.0f, 100.0f);

    CHECK_FALSE(stamina.IsFull());
}
