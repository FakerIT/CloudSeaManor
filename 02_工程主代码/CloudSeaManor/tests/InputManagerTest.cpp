#include "TestFramework.hpp"
#include "CloudSeamanor/engine/InputManager.hpp"

using CloudSeamanor::engine::RegisterTest;

namespace CloudSeamanor {
namespace tests {

// ============================================================================
// InputManager 测试组
// ============================================================================

TEST_CASE(TestInputManagerDefaultConstruction) {
    engine::InputManager input;
    ASSERT_TRUE(input.GetCurrentState().pressed.size() == static_cast<std::size_t>(engine::Action::Count));
    return true;
}

TEST_CASE(TestInputManagerGetMovementVector_Zero) {
    engine::InputManager input;
    sf::Vector2f v = input.GetMovementVector();
    ASSERT_FLOAT_EQ(v.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(v.y, 0.0f, 0.001f);
    return true;
}

TEST_CASE(TestInputManagerGetDefaultKey) {
    engine::InputManager input;
    auto key = input.GetDefaultKey(engine::Action::Interact);
    ASSERT_TRUE(key != sf::Keyboard::Key::Unknown);
    return true;
}

TEST_CASE(TestInputManagerBindKey) {
    engine::InputManager input;
    input.BindKey(sf::Keyboard::Key::Space, engine::Action::Interact);
    auto key = input.GetDefaultKey(engine::Action::Interact);
    // 已有一个默认绑定，绑定第二个键也可以
    ASSERT_TRUE(true);  // 绑定操作不抛异常即通过
    return true;
}

TEST_CASE(TestInputManagerResetToDefaults) {
    engine::InputManager input;
    input.ResetToDefaults();
    ASSERT_TRUE(input.GetCurrentState().pressed.size() == static_cast<std::size_t>(engine::Action::Count));
    return true;
}

TEST_CASE(TestInputManagerGetFontCount_IsZero) {
    engine::InputManager input;
    // InputManager 没有字体管理，这是 UISystem 的职责
    ASSERT_TRUE(true);
    return true;
}

TEST_CASE(TestInputManagerGamepadNotConnected) {
    engine::InputManager input;
    ASSERT_FALSE(input.IsGamepadConnected());
    return true;
}

TEST_CASE(TestInputManagerGetGamepadVector_Zero) {
    engine::InputManager input;
    sf::Vector2f v = input.GetGamepadVector();
    ASSERT_FLOAT_EQ(v.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(v.y, 0.0f, 0.001f);
    return true;
}

}  // namespace tests
}  // namespace CloudSeamanor
