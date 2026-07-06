#pragma once
// Minimal dependency-free test framework used when GoogleTest is not
// available in the build environment. If GTest IS found, tests/CMakeLists.txt
// links against it instead and these macros are unused.
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <sstream>

namespace minitest {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct AssertionFailure {
    std::string message;
};

inline int runAll() {
    int passed = 0, failed = 0;
    for (auto& t : registry()) {
        try {
            t.fn();
            std::cout << "[ PASS ] " << t.name << "\n";
            ++passed;
        } catch (const AssertionFailure& e) {
            std::cout << "[ FAIL ] " << t.name << " -- " << e.message << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[ FAIL ] " << t.name << " -- unexpected exception: " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed, "
              << registry().size() << " total\n";
    return failed == 0 ? 0 : 1;
}

} // namespace minitest

#define TEST(suite, name) \
    void suite##_##name##_impl(); \
    static minitest::Registrar suite##_##name##_reg(#suite "." #name, suite##_##name##_impl); \
    void suite##_##name##_impl()

#define EXPECT_TRUE(cond) \
    if (!(cond)) { std::ostringstream _oss; _oss << "EXPECT_TRUE failed: " #cond; \
        throw minitest::AssertionFailure{_oss.str()}; }

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(a, b) \
    if (!((a) == (b))) { std::ostringstream _oss; _oss << "EXPECT_EQ failed: " #a " != " #b \
        << " (" << (a) << " vs " << (b) << ")"; throw minitest::AssertionFailure{_oss.str()}; }

#define EXPECT_NE(a, b) \
    if (!((a) != (b))) { std::ostringstream _oss; _oss << "EXPECT_NE failed: " #a " == " #b; \
        throw minitest::AssertionFailure{_oss.str()}; }

#define EXPECT_LE(a, b) \
    if (!((a) <= (b))) { std::ostringstream _oss; _oss << "EXPECT_LE failed: " #a " > " #b; \
        throw minitest::AssertionFailure{_oss.str()}; }

#define EXPECT_GT(a, b) \
    if (!((a) > (b))) { std::ostringstream _oss; _oss << "EXPECT_GT failed: " #a " <= " #b; \
        throw minitest::AssertionFailure{_oss.str()}; }
