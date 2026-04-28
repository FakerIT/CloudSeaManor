#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

// SFML 基础类型（用于 InputManager 测试）
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <type_traits>
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

    std::cout << "========================================\n";
    std::cout << " CloudSeamanor Test Suite\n";
    std::cout << "========================================\n";
    std::cout << '\n';

    for (const auto& test : g_tests) {
        std::cout << "Running: " << test.name << " ... ";
        if (test.func()) {
            std::cout << "PASSED\n";
            passed++;
        } else {
            std::cout << "FAILED\n";
            failed++;
        }
    }

    std::cout << '\n';
    std::cout << "========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "========================================\n";

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
            std::cerr << "Assertion failed: " << #condition << '\n'; \
            return false; \
        } \
    } while (false)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "Assertion failed: " << #a << " == " << #b << '\n'; \
            auto _print_value = [](auto&& v) { \
                using V = std::decay_t<decltype(v)>; \
                if constexpr (std::is_enum_v<V>) { \
                    std::cerr << static_cast<int>(v); \
                } else { \
                    std::cerr << v; \
                } \
            }; \
            std::cerr << "  " << #a << " = "; _print_value((a)); std::cerr << '\n'; \
            std::cerr << "  " << #b << " = "; _print_value((b)); std::cerr << '\n'; \
            return false; \
        } \
    } while (false)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "Assertion failed: " << #a << " != " << #b << '\n'; \
            return false; \
        } \
    } while (false)

#define ASSERT_FLOAT_EQ(a, b, epsilon) \
    do { \
        float _a = (a); float _b = (b); \
        if (std::abs(_a - _b) > (epsilon)) { \
            std::cerr << "Assertion failed: " << #a << " ~= " << #b << '\n'; \
            std::cerr << "  " << #a << " = " << _a << '\n'; \
            std::cerr << "  " << #b << " = " << _b << '\n'; \
            return false; \
        } \
    } while (false)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::cerr << "Assertion failed: !" << #condition << '\n'; \
            return false; \
        } \
    } while (false)

} // namespace engine
} // namespace CloudSeamanor

#endif // TEST_FRAMEWORK_HPP
