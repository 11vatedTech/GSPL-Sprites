#include "gspl/studio/theme.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <random>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <stdexcept>

using namespace gspl::studio;

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) { throw std::runtime_error("assertion failed: " #cond); } } while(0)
#define ASSERT_NO_THROW(expr) do { try { expr; } catch (...) { throw std::runtime_error("expected no exception: " #expr); } } while(0)

void test_fuzz_random_hex_color_parsing() {
    std::mt19937 rng(5555);
    std::uniform_int_distribution<uint32_t> hex(0x000000, 0xFFFFFF);

    for (int i = 0; i < 100; ++i) {
        uint32_t rgb = hex(rng);
        std::ostringstream oss;
        oss << "#" << std::hex << std::setfill('0') << std::setw(6) << rgb;
        ASSERT_NO_THROW(ThemeColor::from_hex(oss.str()));
    }
}

void test_fuzz_random_contrast_ratio_bounds() {
    std::mt19937 rng(7777);
    std::uniform_int_distribution<int> channel(0, 255);

    for (int i = 0; i < 100; ++i) {
        auto ch = [&]() { return static_cast<uint8_t>(channel(rng)); };
        ThemeColor a{ch(), ch(), ch(), 255};
        ThemeColor b{ch(), ch(), ch(), 255};
        auto ratio = a.contrast_ratio(b);
        ASSERT(ratio >= 1.0);
        ASSERT(ratio <= 21.0);
    }
}

void test_fuzz_theme_loading_and_activation() {
    ThemeManager manager;
    manager.load_builtin_themes();

    for (const auto& theme : manager.available_themes()) {
        ASSERT_NO_THROW(manager.activate_theme(theme.id));
    }
}

void test_fuzz_rapid_theme_switching() {
    ThemeManager manager;
    manager.load_builtin_themes();
    auto themes = manager.available_themes();
    ASSERT(!themes.empty());

    std::mt19937 rng(1111);
    std::uniform_int_distribution<size_t> dist(0, themes.size() - 1);

    for (int i = 0; i < 200; ++i) {
        auto& theme = themes[dist(rng)];
        ASSERT_NO_THROW(manager.activate_theme(theme.id));
    }
}

int main() {
    std::printf("Fuzz Tests (Studio)\n");

    TEST(test_fuzz_random_hex_color_parsing);
    TEST(test_fuzz_random_contrast_ratio_bounds);
    TEST(test_fuzz_theme_loading_and_activation);
    TEST(test_fuzz_rapid_theme_switching);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
