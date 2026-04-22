// ============================================================================
// 【catch2Compat.hpp】Catch2 v3 兼容测试框架（嵌入式实现）
// ============================================================================
// API 兼容 Catch2 v3，测试代码无需修改，网络恢复后一行切换到真实 Catch2。
//
// 支持的 API：
//   TEST_CASE(name) / SECTION(name) / GIVEN/WHEN/THEN
//   REQUIRE(x) / CHECK(x) / REQUIRE_FALSE(x) / CHECK_FALSE(x)
//   REQUIRE_EQ(a,b) / CHECK_EQ(a,b) / REQUIRE_NE(a,b) / CHECK_NE(a,b)
//   REQUIRE_FLOAT_EQ(a,b,eps) / CHECK_FLOAT_EQ(a,b,eps)
//   REQUIRE_THAT(str, Equals(...)) / StartsWith(...) / Contains(...)
//   REQUIRE_THROW / REQUIRE_NOTHROW
//
// 迁移指南：下载真实 catch2.hpp 后，将第 1 行注释掉：
//   // #define CATCH2_EMBEDDED_IMPLEMENTATION
//   #include <catch2/catch_test_macros.hpp>
// ============================================================================

#pragma once

// ---- 开关：注释此行即可切换到真实 Catch2 ----
#define CATCH2_EMBEDDED_IMPLEMENTATION
// #include <catch2/catch_test_macros.hpp>

// ============================================================================
// 嵌入式实现
// ============================================================================
#ifdef CATCH2_EMBEDDED_IMPLEMENTATION

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---- 辅助宏 ----
#define CATCH2_CONCAT_IMPL(a, b) a##b
#define CATCH2_CONCAT(a, b) CATCH2_CONCAT_IMPL(a, b)
#define CATCH2_STRINGIFY(x) #x

// ============================================================================
// StringMaker（值转字符串）
// ============================================================================
namespace CloudSeamanor {
namespace testing {

template <typename T>
struct StringMaker {
    static std::string toString(const T& v) {
        std::ostringstream oss; oss << v; return oss.str();
    }
};

template <> struct StringMaker<float> {
    static std::string toString(float v) {
        std::ostringstream oss; oss << v << "f"; return oss.str();
    }
};
template <> struct StringMaker<int> {
    static std::string toString(int v) { return std::to_string(v); }
};
template <> struct StringMaker<unsigned> {
    static std::string toString(unsigned v) { return std::to_string(v); }
};
template <> struct StringMaker<bool> {
    static std::string toString(bool v) { return v ? "true" : "false"; }
};
template <> struct StringMaker<std::string> {
    static std::string toString(const std::string& v) { return "\"" + v + "\""; }
};
template <> struct StringMaker<const char*> {
    static std::string toString(const char* v) {
        return v ? std::string("\"") + v + "\"" : "nullptr";
    }
};

// ============================================================================
// AssertionResult
// ============================================================================
struct AssertionResult {
    bool success = false;
    std::string message;
    std::string file;
    int line = 0;
    static AssertionResult ok() { return {true, "", "", 0}; }
    static AssertionResult fail(const std::string& msg, const std::string& f, int l) {
        return {false, msg, f, l};
    }
};

// ============================================================================
// TestFailure
// ============================================================================
struct TestFailure {
    std::string suite;
    std::string test;
    std::string expr;
    std::string message;
    std::string file;
    int line = 0;
    bool is_requirement = false;

    std::string format() const {
        std::ostringstream oss;
        oss << "\n";
        if (!suite.empty()) oss << suite << " :: ";
        oss << test << "\n";
        if (!expr.empty())   oss << "  Expression: " << expr << "\n";
        if (!message.empty()) oss << "  " << message << "\n";
        oss << "  at " << file << ":" << line << "\n";
        return oss.str();
    }
};

// ============================================================================
// TestSuite
// ============================================================================
struct TestSuite {
    std::string name;
    int passed = 0;
    int failed = 0;
    int warnings = 0;
    std::vector<TestFailure> failures;
    std::vector<TestFailure> warnings_list;
};

// ============================================================================
// TestCase
// ============================================================================
struct TestCase {
    std::string name;
    std::string suite;
    std::function<void()> run;
    std::string source_file;
    int source_line = 0;
};

// ============================================================================
// GlobalRegistry
// ============================================================================
struct GlobalRegistry {
    static GlobalRegistry& instance() {
        static GlobalRegistry reg;
        return reg;
    }

    std::vector<TestCase> cases;
    std::vector<TestSuite> suites;
    std::string current_suite;
    std::string current_test;
    int total_passed = 0;
    int total_failed = 0;
    bool show_success = false;

    void ensure_suite_exists(const std::string& name) {
        for (auto& s : suites) if (s.name == name) return;
        suites.push_back({name});
    }

    TestSuite* current_suite_ptr = nullptr;

    TestSuite* get_current_suite() {
        if (!current_suite.empty()) {
            for (auto& s : suites) if (s.name == current_suite) return &s;
        }
        ensure_suite_exists("<unknown>");
        return &suites.back();
    }
};

// ============================================================================
// Matchers
// ============================================================================
class MatcherBase {
public:
    virtual ~MatcherBase() = default;
    virtual bool match(const std::string&) const = 0;
    virtual std::string describe() const = 0;
};

class EqualsMatcher : public MatcherBase {
    std::string expected_;
public:
    explicit EqualsMatcher(const std::string& e) : expected_(e) {}
    bool match(const std::string& v) const override { return v == expected_; }
    std::string describe() const override {
        return std::string("equals \"") + expected_ + "\"";
    }
};

class StartsWithMatcher : public MatcherBase {
    std::string prefix_;
public:
    explicit StartsWithMatcher(const std::string& p) : prefix_(p) {}
    bool match(const std::string& v) const override {
        return v.size() >= prefix_.size() && v.substr(0, prefix_.size()) == prefix_;
    }
    std::string describe() const override {
        return std::string("starts with \"") + prefix_ + "\"";
    }
};

class ContainsMatcher : public MatcherBase {
    std::string substr_;
public:
    explicit ContainsMatcher(const std::string& s) : substr_(s) {}
    bool match(const std::string& v) const override {
        return v.find(substr_) != std::string::npos;
    }
    std::string describe() const override {
        return std::string("contains \"") + substr_ + "\"";
    }
};

[[nodiscard]] inline EqualsMatcher Equals(const std::string& e) { return EqualsMatcher(e); }
[[nodiscard]] inline StartsWithMatcher StartsWith(const std::string& p) { return StartsWithMatcher(p); }
[[nodiscard]] inline ContainsMatcher Contains(const std::string& s) { return ContainsMatcher(s); }

}  // namespace testing
}  // namespace CloudSeamanor

// Catch2-style unqualified matcher factories used by existing tests.
using CloudSeamanor::testing::Contains;
using CloudSeamanor::testing::Equals;
using CloudSeamanor::testing::StartsWith;
namespace CloudSeamanor {
namespace testing {

// ============================================================================
// TestFailureException
// ============================================================================
struct TestFailureException : std::exception {
    explicit TestFailureException(const TestFailure& f) : failure(f) {}
    const TestFailure& failure;
};

// ============================================================================
// 运行器
// ============================================================================
inline void print_report() {
    auto& reg = GlobalRegistry::instance();
    std::cout << "\n========================================\n";
    std::cout << " CloudSeamanor Test Suite\n";
    std::cout << "========================================\n\n";

    int total_failed = 0;
    for (const auto& suite : reg.suites) {
        if (suite.name.empty()) continue;
        if (!suite.failures.empty()) {
            std::cout << "  FAILED suite: " << suite.name << "\n";
            for (const auto& f : suite.failures) {
                std::cout << f.format();
                total_failed++;
            }
        }
        if (!suite.warnings_list.empty()) {
            std::cout << "  WARN suite: " << suite.name << "\n";
            for (const auto& w : suite.warnings_list) {
                std::cout << w.format();
            }
        }
    }
    std::cout << "========================================\n";
    if (total_failed == 0) {
        std::cout << "  All tests passed (" << reg.total_passed << ")\n";
    } else {
        std::cout << "  " << total_failed << " test(s) FAILED\n";
    }
    std::cout << "========================================\n";
}

inline int run_all_tests(int argc, char* argv[]) {
    auto& reg = GlobalRegistry::instance();
    (void)argc; (void)argv;

    for (auto& test_case : reg.cases) {
        reg.ensure_suite_exists(test_case.suite);
        reg.current_suite = test_case.suite;
        reg.current_test = test_case.name;
        auto* suite = reg.get_current_suite();

        bool test_failed = false;
        try {
            test_case.run();
        } catch (const TestFailureException& e) {
            suite->failures.push_back(e.failure);
            test_failed = true;
        } catch (const std::exception& e) {
            TestFailure fail;
            fail.suite = test_case.suite;
            fail.test = test_case.name;
            fail.message = std::string("Unhandled exception: ") + e.what();
            fail.is_requirement = true;
            suite->failures.push_back(fail);
            test_failed = true;
        } catch (...) {
            TestFailure fail;
            fail.suite = test_case.suite;
            fail.test = test_case.name;
            fail.message = "Unhandled unknown exception";
            fail.is_requirement = true;
            suite->failures.push_back(fail);
            test_failed = true;
        }

        if (!suite->failures.empty()) {
            bool had_req_fail = false;
            for (const auto& f : suite->failures) {
                if (f.is_requirement) had_req_fail = true;
            }
            if (had_req_fail) {
                suite->failed++;
                reg.total_failed++;
            }
        } else {
            suite->passed++;
            reg.total_passed++;
        }
    }

    print_report();
    return (reg.total_failed > 0) ? 1 : 0;
}

}  // namespace testing
}  // namespace CloudSeamanor

// ============================================================================
// 公共宏
// ============================================================================

// ---- TEST_CASE ----
// Some legacy tests still pull in TestFramework.hpp, which also defines TEST_CASE.
// Undefine first to ensure Catch2-compatible syntax (string literal names) works.
#ifdef TEST_CASE
#undef TEST_CASE
#endif
#define TEST_CASE(name) CATCH2_TEST_CASE_IMPL(name, __FILE__, __LINE__)

#define CATCH2_TEST_CASE_IMPL(name, file, line)                                           \
    static void CATCH2_CONCAT(catch2_test_func_, line)();                                 \
    namespace {                                                                           \
    struct CATCH2_CONCAT(TestRegistrar_, line) {                                          \
        CATCH2_CONCAT(TestRegistrar_, line)() {                                           \
            auto& reg = CloudSeamanor::testing::GlobalRegistry::instance();               \
            CloudSeamanor::testing::TestCase tc;                                          \
            std::string full_name = (name);                                               \
            const auto sep = full_name.find("::");                                        \
            if (sep != std::string::npos) {                                               \
                tc.suite = full_name.substr(0, sep);                                      \
                tc.name = full_name.substr(sep + 2);                                      \
            } else {                                                                      \
                tc.suite.clear();                                                         \
                tc.name = full_name;                                                      \
            }                                                                             \
            tc.source_file = (file);                                                      \
            tc.source_line = (line);                                                      \
            tc.run = &CATCH2_CONCAT(catch2_test_func_, line);                             \
            reg.cases.push_back(std::move(tc));                                           \
        }                                                                                 \
    };                                                                                    \
    static CATCH2_CONCAT(TestRegistrar_, line) CATCH2_CONCAT(test_reg_inst_, line);       \
    }                                                                                     \
    static void CATCH2_CONCAT(catch2_test_func_, line)()

// ---- SECTION（简化实现）----
#define SECTION(name) \
    for (bool _cs_done = false; !_cs_done; )

// ---- GIVEN/WHEN/THEN ----
#define GIVEN(desc)      SECTION(std::string("Given: ") + desc)
#define WHEN(desc)       SECTION(std::string("When: ") + desc)
#define THEN(desc)       SECTION(std::string("Then: ") + desc)
#define AND_THEN(desc)   SECTION(std::string("And: ") + desc)

// ============================================================================
// 内部辅助：断言失败处理
// ============================================================================
namespace CloudSeamanor {
namespace testing {
namespace detail {

inline void handle_assertion_fail(
    const std::string& expr,
    const std::string& message,
    const std::string& file,
    int line,
    bool is_requirement
) {
    auto& reg = GlobalRegistry::instance();
    TestFailure fail;
    fail.suite = reg.current_suite;
    fail.test = reg.current_test;
    fail.expr = expr;
    fail.message = message;
    fail.file = file;
    fail.line = line;
    fail.is_requirement = is_requirement;

    auto* suite = reg.get_current_suite();
    if (is_requirement) {
        suite->failures.push_back(fail);
        throw TestFailureException(fail);
    } else {
        suite->warnings_list.push_back(fail);
    }
}

}  // namespace detail
}  // namespace testing
}  // namespace CloudSeamanor

// ---- REQUIRE / CHECK ----
#define REQUIRE(expr) \
    do { \
        auto _val = static_cast<bool>(expr); \
        if (!_val) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "", __FILE__, __LINE__, true); \
        } \
    } while (false)

#define CHECK(expr) \
    do { \
        auto _val = static_cast<bool>(expr); \
        if (!_val) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "", __FILE__, __LINE__, false); \
        } \
    } while (false)

#define REQUIRE_FALSE(expr) \
    do { \
        auto _val = static_cast<bool>(expr); \
        if (_val) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "Expected false", __FILE__, __LINE__, true); \
        } \
    } while (false)

#define CHECK_FALSE(expr) \
    do { \
        auto _val = static_cast<bool>(expr); \
        if (_val) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "Expected false", __FILE__, __LINE__, false); \
        } \
    } while (false)

#define CHECK_TRUE(expr) \
    do { \
        auto _val = static_cast<bool>(expr); \
        if (!_val) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "Expected true", __FILE__, __LINE__, false); \
        } \
    } while (false)

// ---- REQUIRE_EQ / CHECK_EQ ----
#define CATCH2_IMPL_EQ(a, b, is_req, op, op_str) \
    do { \
        auto _ea = (a); \
        auto _eb = (b); \
        if (!(_ea op _eb)) { \
            std::ostringstream _oss; \
            _oss << #a << " = " << CloudSeamanor::testing::StringMaker<decltype(_ea)>::toString(_ea) \
                 << " vs " #op " " #b << " = " << CloudSeamanor::testing::StringMaker<decltype(_eb)>::toString(_eb); \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                std::string(#a) + " " op_str " " + #b, _oss.str(), __FILE__, __LINE__, is_req); \
        } \
    } while (false)

#define REQUIRE_EQ(a, b)  CATCH2_IMPL_EQ(a, b, true,  ==, "==")
#define CHECK_EQ(a, b)    CATCH2_IMPL_EQ(a, b, false, ==, "==")
#define REQUIRE_NE(a, b)  CATCH2_IMPL_EQ(a, b, true,  !=, "!=")
#define CHECK_NE(a, b)    CATCH2_IMPL_EQ(a, b, false, !=, "!=")

#define REQUIRE_LE(a, b)  CATCH2_IMPL_EQ(a, b, true,  <=, "<=")
#define CHECK_LE(a, b)    CATCH2_IMPL_EQ(a, b, false, <=, "<=")

// ---- REQUIRE_FLOAT_EQ / CHECK_FLOAT_EQ ----
#define CATCH2_IMPL_FLOAT_EQ(a, b, eps, is_req) \
    do { \
        auto _fa = static_cast<double>(a); \
        auto _fb = static_cast<double>(b); \
        auto _fe = static_cast<double>(eps); \
        if (std::abs(_fa - _fb) > _fe) { \
            std::ostringstream _oss; \
            _oss << #a << " = " << _fa << " vs " #b << " = " << _fb \
                 << ", epsilon = " << _fe; \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                "float approx(" #a ", " #b ", " #eps ")", _oss.str(), __FILE__, __LINE__, is_req); \
        } \
    } while (false)

#define REQUIRE_FLOAT_EQ(a, b, eps) CATCH2_IMPL_FLOAT_EQ(a, b, eps, true)
#define CHECK_FLOAT_EQ(a, b, eps)   CATCH2_IMPL_FLOAT_EQ(a, b, eps, false)

// ---- REQUIRE_THAT ----
#define REQUIRE_THAT(str_expr, matcher_expr) \
    do { \
        auto _str_val = (str_expr); \
        auto _matcher_val = (matcher_expr); \
        if (!_matcher_val.match(_str_val)) { \
            std::ostringstream _oss; \
            _oss << "  " << CATCH2_STRINGIFY(str_expr) << " (\"" << _str_val \
                 << "\") " << _matcher_val.describe(); \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(str_expr), _oss.str(), __FILE__, __LINE__, true); \
        } \
    } while (false)

// ---- CHECK_THAT ----
#define CHECK_THAT(str_expr, matcher_expr) \
    do { \
        auto _str_val = (str_expr); \
        auto _matcher_val = (matcher_expr); \
        if (!_matcher_val.match(_str_val)) { \
            std::ostringstream _oss; \
            _oss << "  " << CATCH2_STRINGIFY(str_expr) << " (\"" << _str_val \
                 << "\") " << _matcher_val.describe(); \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(str_expr), _oss.str(), __FILE__, __LINE__, false); \
        } \
    } while (false)

// ---- REQUIRE_THROW / REQUIRE_NOTHROW ----
#define REQUIRE_THROWS(expr) \
    do { \
        bool _threw = false; std::string _what; \
        try { (void)(expr); } \
        catch (const std::exception& e) { _threw = true; _what = e.what(); } \
        catch (...) { _threw = true; _what = "<unknown>"; } \
        if (!_threw) { \
            std::ostringstream _oss; \
            _oss << "Expected " << CATCH2_STRINGIFY(expr) << " to throw, but it did not"; \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), _oss.str(), __FILE__, __LINE__, true); \
        } \
    } while (false)

#define REQUIRE_NOTHROW(expr) \
    do { \
        try { (void)(expr); } \
        catch (const std::exception& e) { \
            std::ostringstream _oss; \
            _oss << "Unexpected exception: " << e.what(); \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), _oss.str(), __FILE__, __LINE__, true); \
        } \
        catch (...) { \
            CloudSeamanor::testing::detail::handle_assertion_fail( \
                CATCH2_STRINGIFY(expr), "Unexpected unknown exception", __FILE__, __LINE__, true); \
        } \
    } while (false)

// ---- main ----
int main(int argc, char* argv[]);

#endif  // CATCH2_EMBEDDED_IMPLEMENTATION
