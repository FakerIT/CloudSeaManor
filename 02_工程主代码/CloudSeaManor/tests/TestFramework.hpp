#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

// SFML 基础类型（用于 InputManager 测试）
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace CloudSeamanor {
namespace engine {

// ============================================================================
// 【简单测试框架】
// ============================================================================

struct TestCase {
    const char* name;
    bool (*func)();
};

static std::vector<TestCase> g_tests;

void RegisterTest(const char* name, bool (*func)()) {
    g_tests.push_back({name, func});
}

int RunAllTests() {
    int passed = 0;
    int failed = 0;

    std::cout << "========================================" << std::endl;
    std::cout << " CloudSeamanor Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    for (const auto& test : g_tests) {
        std::cout << "Running: " << test.name << " ... ";
        if (test.func()) {
            std::cout << "PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "FAILED" << std::endl;
            failed++;
        }
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed;
}

#define TEST_CASE(name) \
    static bool name(); \
    struct name##_registrar { \
        name##_registrar() { RegisterTest(#name, name); } \
    } name##_instance; \
    static bool name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << std::endl; \
            return false; \
        } \
    } while (false)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "Assertion failed: " << #a << " == " << #b << std::endl; \
            std::cerr << "  " << #a << " = " << (a) << std::endl; \
            std::cerr << "  " << #b << " = " << (b) << std::endl; \
            return false; \
        } \
    } while (false)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "Assertion failed: " << #a << " != " << #b << std::endl; \
            return false; \
        } \
    } while (false)

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
    do { \
        float _a = (a); float _b = (b); \
        if (std::abs(_a - _b) > (epsilon)) { \
            std::cerr << "Assertion failed: " << #a << " ~= " << #b << std::endl; \
            std::cerr << "  " << #a << " = " << _a << std::endl; \
            std::cerr << "  " << #b << " = " << _b << std::endl; \
            return false; \
        } \
    } while (false)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::cerr << "Assertion failed: !" << #condition << std::endl; \
            return false; \
        } \
    } while (false)

} // namespace engine
} // namespace CloudSeamanor

#endif // TEST_FRAMEWORK_HPP
