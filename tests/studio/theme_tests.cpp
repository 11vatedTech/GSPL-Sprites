#include "gspl/studio/theme.hpp"
#include <cassert>
#include <cstdio>

void test_theme_color_from_hex() {
    auto c = gspl::studio::ThemeColor::from_hex("#ff6600");
    assert(c.r == 255);
    assert(c.g == 102);
    assert(c.b == 0);
    assert(c.a == 255);
}

void test_theme_color_to_hex() {
    gspl::studio::ThemeColor c{255, 102, 0, 255};
    auto hex = c.to_hex();
    assert(hex == "#ff6600");
}

void test_theme_manager_builtins() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    assert(mgr.available_themes().size() >= 3);
}

void test_theme_activation() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    assert(mgr.activate_theme("dark"));
    assert(mgr.active_theme().id == "dark");
    assert(mgr.active_theme().is_dark);
}

void test_theme_contrast_ratio() {
    gspl::studio::ThemeColor black{0, 0, 0, 255};
    gspl::studio::ThemeColor white{255, 255, 255, 255};
    auto ratio = black.contrast_ratio(white);
    assert(ratio > 20.0f);
}

int main() {
    test_theme_color_from_hex();
    test_theme_color_to_hex();
    test_theme_manager_builtins();
    test_theme_activation();
    test_theme_contrast_ratio();
    std::printf("All theme tests passed.\n");
    return 0;
}
