#include "TestFramework.hpp"
#include "CloudSeamanor/Stamina.hpp"

using CloudSeamanor::engine::RegisterTest;

namespace CloudSeamanor {
namespace tests {

// ============================================================================
// Stamina 测试组
// ============================================================================

TEST_CASE(TestStaminaInitialValues) {
    domain::StaminaSystem stamina;
    stamina.Initialize(350.0f, 350.0f);
    ASSERT_FLOAT_EQ(stamina.Max(), 350.0f, 0.001f);
    ASSERT_FLOAT_EQ(stamina.Current(), 350.0f, 0.001f);
    ASSERT_FLOAT_EQ(stamina.Ratio(), 1.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaConsume) {
    domain::StaminaSystem stamina;
    stamina.Initialize(100.0f, 100.0f);
    stamina.Consume(30.0f);
    ASSERT_FLOAT_EQ(stamina.Current(), 70.0f, 0.001f);
    ASSERT_FLOAT_EQ(stamina.Ratio(), 0.7f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaConsumeCannotGoBelowZero) {
    domain::StaminaSystem stamina;
    stamina.Initialize(10.0f, 100.0f);
    stamina.Consume(50.0f);
    ASSERT_FLOAT_EQ(stamina.Current(), 0.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaRecover) {
    domain::StaminaSystem stamina;
    stamina.Initialize(30.0f, 100.0f);
    stamina.Recover(20.0f);
    ASSERT_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaRecoverCannotExceedMax) {
    domain::StaminaSystem stamina;
    stamina.Initialize(90.0f, 100.0f);
    stamina.Recover(50.0f);
    ASSERT_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaRefill) {
    domain::StaminaSystem stamina;
    stamina.Initialize(25.0f, 100.0f);
    stamina.Refill();
    ASSERT_FLOAT_EQ(stamina.Current(), 100.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaRatioBoundary) {
    domain::StaminaSystem stamina;
    stamina.Initialize(0.0f, 200.0f);
    ASSERT_FLOAT_EQ(stamina.Ratio(), 0.0f, 0.001f);
    stamina.SetCurrent(200.0f);
    ASSERT_FLOAT_EQ(stamina.Ratio(), 1.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaNegativeConsumeIsNoOp) {
    domain::StaminaSystem stamina;
    stamina.Initialize(50.0f, 100.0f);
    stamina.Consume(-10.0f);
    ASSERT_FLOAT_EQ(stamina.Current(), 50.0f, 0.001f);
    return true;
}

TEST_CASE(TestStaminaSetMaxAndCurrent) {
    domain::StaminaSystem stamina;
    stamina.Initialize(120.0f, 350.0f);
    stamina.SetMax(500.0f);
    stamina.SetCurrent(200.0f);
    ASSERT_FLOAT_EQ(stamina.Max(), 500.0f, 0.001f);
    ASSERT_FLOAT_EQ(stamina.Current(), 200.0f, 0.001f);
    return true;
}

}  // namespace tests
}  // namespace CloudSeamanor
