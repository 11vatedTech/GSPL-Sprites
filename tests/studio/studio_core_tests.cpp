#include "gspl/studio/publishing.hpp"
#include "gspl/studio/target_adapter.hpp"
#include "gspl/studio/theme.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <cmath>
#include <vector>
#include <stdexcept>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    ++tests_run; \
    try { \
        name(); \
        std::printf("  PASS  %s\n", #name); \
    } catch (const std::exception& e) { \
        std::printf("  FAIL  %s: %s\n", #name, e.what()); \
        ++tests_failed; \
    } catch (...) { \
        std::printf("  FAIL  %s: unknown exception\n", #name); \
        ++tests_failed; \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("assertion failed: " #cond); \
    } \
} while(0)

// --- Publishing Manager ---

static void test_publishing_manager_create() {
    gspl::studio::PublishingManager mgr;
    ASSERT(mgr.targets().empty());
}

static void test_publishing_manager_add_target() {
    gspl::studio::PublishingManager mgr;
    gspl::studio::PublishTarget target;
    target.id = "test-target";
    target.name = "Test Target";
    target.type = "local-registry";

    bool ok = mgr.add_target(target);
    ASSERT(ok);
    ASSERT(mgr.targets().size() == 1);
}

static void test_publishing_manager_add_duplicate() {
    gspl::studio::PublishingManager mgr;
    gspl::studio::PublishTarget target;
    target.id = "dup";
    target.name = "Original";
    target.type = "local-registry";

    ASSERT(mgr.add_target(target));
    ASSERT(!mgr.add_target(target));
    ASSERT(mgr.targets().size() == 1);
}

static void test_publishing_manager_remove() {
    gspl::studio::PublishingManager mgr;
    gspl::studio::PublishTarget target;
    target.id = "remove-me";
    target.name = "Remove Me";
    target.type = "local-registry";

    ASSERT(mgr.add_target(target));
    ASSERT(mgr.remove_target("remove-me"));
    ASSERT(mgr.targets().empty());
}

static void test_publishing_manager_remove_nonexistent() {
    gspl::studio::PublishingManager mgr;
    ASSERT(!mgr.remove_target("does-not-exist"));
}

static void test_publishing_manager_publish_no_target() {
    gspl::studio::PublishingManager mgr;
    auto result = mgr.publish("pkg.gspl", "nonexistent", gspl::studio::PublishChannel::Stable, "test");
    ASSERT(!result.success);
}

static void test_publishing_manager_rollback_no_target() {
    gspl::studio::PublishingManager mgr;
    auto result = mgr.rollback("nonexistent", "1.0.0");
    ASSERT(!result.success);
}

// --- Target Adapter Manager ---

static void test_target_adapter_create() {
    gspl::studio::TargetAdapterManager mgr;
    ASSERT(mgr.adapters().empty());
}

static void test_target_adapter_register() {
    gspl::studio::TargetAdapterManager mgr;
    gspl::studio::TargetAdapterInfo info;
    info.id = "test-adapter";
    info.name = "Test";
    info.version = "1.0";

    ASSERT(mgr.register_adapter(info));
    ASSERT(mgr.adapters().size() == 1);
}

static void test_target_adapter_register_duplicate() {
    gspl::studio::TargetAdapterManager mgr;
    gspl::studio::TargetAdapterInfo info;
    info.id = "dup";
    info.name = "Dup";

    ASSERT(mgr.register_adapter(info));
    ASSERT(!mgr.register_adapter(info));
    ASSERT(mgr.adapters().size() == 1);
}

static void test_target_adapter_unregister() {
    gspl::studio::TargetAdapterManager mgr;
    gspl::studio::TargetAdapterInfo info;
    info.id = "rm";
    info.name = "Remove";

    ASSERT(mgr.register_adapter(info));
    ASSERT(mgr.unregister_adapter("rm"));
    ASSERT(mgr.adapters().empty());
}

static void test_target_adapter_find_nonexistent() {
    gspl::studio::TargetAdapterManager mgr;
    ASSERT(mgr.find("nothing") == nullptr);
}

static void test_target_adapter_find_registered() {
    gspl::studio::TargetAdapterManager mgr;
    gspl::studio::TargetAdapterInfo info;
    info.id = "found-me";
    info.name = "Found";

    ASSERT(mgr.register_adapter(info));
    auto* found = mgr.find("found-me");
    ASSERT(found != nullptr);
    ASSERT(found->id == "found-me");
}

static void test_target_adapter_load_builtin() {
    gspl::studio::TargetAdapterManager mgr;
    mgr.load_builtin_adapters();
    ASSERT(mgr.adapters().size() >= 1);
}

static void test_target_adapter_detect_sdk_nonexistent() {
    gspl::studio::TargetAdapterManager mgr;
    ASSERT(!mgr.detect_sdk("nonexistent"));
}

static void test_target_adapter_active_profile_empty() {
    gspl::studio::TargetAdapterManager mgr;
    ASSERT(mgr.active_profile("nonexistent").empty());
}

static void test_target_adapter_set_active_profile_nonexistent() {
    gspl::studio::TargetAdapterManager mgr;

    gspl::studio::TargetAdapterInfo info;
    info.id = "prof-test";
    info.name = "Profile Test";
    gspl::studio::TargetProfile p;
    p.name = "debug";
    info.profiles.push_back(p);
    mgr.register_adapter(info);

    ASSERT(!mgr.set_active_profile("prof-test", "nope"));
    ASSERT(mgr.set_active_profile("prof-test", "debug"));
}

// --- Theme ---

static void test_theme_color_to_hex_red() {
    auto hex = gspl::studio::ThemeColor{255, 0, 0}.to_hex();
    ASSERT(hex == "#FF0000");
}

static void test_theme_color_to_hex_white() {
    auto hex = gspl::studio::ThemeColor{255, 255, 255}.to_hex();
    ASSERT(hex == "#FFFFFF");
}

static void test_theme_color_to_hex_black() {
    auto hex = gspl::studio::ThemeColor{0, 0, 0}.to_hex();
    ASSERT(hex == "#000000");
}

static void test_theme_color_from_hex_red() {
    auto c = gspl::studio::ThemeColor::from_hex("#ff0000");
    ASSERT(c.r == 255);
    ASSERT(c.g == 0);
    ASSERT(c.b == 0);
}

static void test_theme_color_from_hex_defaults() {
    auto c = gspl::studio::ThemeColor::from_hex("#000000");
    ASSERT(c.r == 0);
    ASSERT(c.g == 0);
    ASSERT(c.b == 0);
}

static void test_theme_contrast_ratio_same() {
    auto c = gspl::studio::ThemeColor::from_hex("#000000");
    float ratio = c.contrast_ratio(c);
    ASSERT(std::abs(ratio - 1.0f) < 0.001f);
}

static void test_theme_contrast_ratio_black_white() {
    auto black = gspl::studio::ThemeColor::from_hex("#000000");
    auto white = gspl::studio::ThemeColor::from_hex("#ffffff");
    float ratio = black.contrast_ratio(white);
    ASSERT(ratio > 20.0f);
    ASSERT(ratio < 22.0f);
}

static void test_theme_manager_load_builtin() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    ASSERT(mgr.available_themes().size() >= 3);
}

static void test_theme_manager_activate_builtin() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    ASSERT(mgr.activate_theme("gspl-dark"));
    ASSERT(mgr.active_theme().id == "gspl-dark");
}

static void test_theme_manager_activate_light() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    ASSERT(mgr.activate_theme("gspl-light"));
    ASSERT(mgr.active_theme().is_dark == false);
}

static void test_theme_manager_activate_high_contrast() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    ASSERT(mgr.activate_theme("gspl-high-contrast"));
    ASSERT(mgr.active_theme().is_high_contrast);
}

static void test_theme_manager_find() {
    gspl::studio::ThemeManager mgr;
    mgr.load_builtin_themes();
    auto* t = mgr.find("gspl-dark");
    ASSERT(t != nullptr);
    ASSERT(t->id == "gspl-dark");
}

static void test_theme_manager_activate_nonexistent() {
    gspl::studio::ThemeManager mgr;
    ASSERT(!mgr.activate_theme("does-not-exist"));
}

int main() {
    std::printf("=== Studio Core Tests ===\n\n");

    // Publishing Manager
    TEST(test_publishing_manager_create);
    TEST(test_publishing_manager_add_target);
    TEST(test_publishing_manager_add_duplicate);
    TEST(test_publishing_manager_remove);
    TEST(test_publishing_manager_remove_nonexistent);
    TEST(test_publishing_manager_publish_no_target);
    TEST(test_publishing_manager_rollback_no_target);

    // Target Adapter Manager
    TEST(test_target_adapter_create);
    TEST(test_target_adapter_register);
    TEST(test_target_adapter_register_duplicate);
    TEST(test_target_adapter_unregister);
    TEST(test_target_adapter_find_nonexistent);
    TEST(test_target_adapter_find_registered);
    TEST(test_target_adapter_load_builtin);
    TEST(test_target_adapter_detect_sdk_nonexistent);
    TEST(test_target_adapter_active_profile_empty);
    TEST(test_target_adapter_set_active_profile_nonexistent);

    // Theme
    TEST(test_theme_color_to_hex_red);
    TEST(test_theme_color_to_hex_white);
    TEST(test_theme_color_to_hex_black);
    TEST(test_theme_color_from_hex_red);
    TEST(test_theme_color_from_hex_defaults);
    TEST(test_theme_contrast_ratio_same);
    TEST(test_theme_contrast_ratio_black_white);
    TEST(test_theme_manager_load_builtin);
    TEST(test_theme_manager_activate_builtin);
    TEST(test_theme_manager_activate_light);
    TEST(test_theme_manager_activate_high_contrast);
    TEST(test_theme_manager_find);
    TEST(test_theme_manager_activate_nonexistent);

    std::printf("\n=== Results: %d tests, %d failed ===\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
