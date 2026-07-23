#include "gspl/studio/environment_provider.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
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

// --- Allowlist ---

static void test_default_allowlist_not_empty() {
    auto list = gspl::studio::EnvironmentProvider::default_allowlist();
    ASSERT(!list.empty());
}

static void test_default_allowlist_contains_home() {
    auto list = gspl::studio::EnvironmentProvider::default_allowlist();
    bool found = false;
    for (const auto& name : list) {
        if (name == "HOME") found = true;
    }
    ASSERT(found);
}

// --- Get unset variable ---

static void test_get_unset_variable() {
    gspl::studio::EnvironmentProvider env({"ALLOWED_VAR"});
    auto entry = env.get("ALLOWED_VAR");
    ASSERT(!entry.is_set);
    ASSERT(entry.value.empty());
}

// --- Get set variable (via override) ---

static void test_get_override() {
    gspl::studio::EnvironmentProvider env({"TEST_VAR"});
    env.set_override("TEST_VAR", "test_value");
    auto entry = env.get("TEST_VAR");
    ASSERT(entry.is_set);
    ASSERT(entry.value == "test_value");
}

// --- Distinguish unset from empty ---

static void test_override_empty_value() {
    gspl::studio::EnvironmentProvider env({"EMPTY_VAR"});
    env.set_override("EMPTY_VAR", "");
    auto entry = env.get("EMPTY_VAR");
    ASSERT(entry.is_set);
    ASSERT(entry.value.empty());
}

// --- Non-allowlisted variable is blocked ---

static void test_non_allowlisted_var_blocked() {
    gspl::studio::EnvironmentProvider env({"ALLOWED"});
    env.set_override("SECRET", "sensitive");
    auto entry = env.get("SECRET");
    ASSERT(!entry.is_set);
    ASSERT(entry.value.empty());
}

// --- Clear overrides ---

static void test_clear_overrides() {
    gspl::studio::EnvironmentProvider env({"TEST"});
    env.set_override("TEST", "value");
    env.clear_overrides();
    auto entry = env.get("TEST");
    ASSERT(!entry.is_set);
}

// --- Update allowlist ---

static void test_set_allowlist() {
    gspl::studio::EnvironmentProvider env({"OLD"});
    env.set_allowlist({"NEW"});
    env.set_override("NEW", "works");
    env.set_override("OLD", "blocked");
    ASSERT(env.get("NEW").is_set);
    ASSERT(!env.get("OLD").is_set);
}

// --- Get_path with invalid path ---

static void test_get_path_invalid() {
    gspl::studio::EnvironmentProvider env({"PATH_VAR"});
    env.set_override("PATH_VAR", "/nonexistent/path/that/does/not/exist");
    auto entry = env.get_path("PATH_VAR");
    ASSERT(entry.is_set);
    ASSERT(!entry.is_valid_path);
}

// --- Get_path with empty value ---

static void test_get_path_empty() {
    gspl::studio::EnvironmentProvider env({"EMPTY_PATH"});
    env.set_override("EMPTY_PATH", "");
    auto entry = env.get_path("EMPTY_PATH");
    ASSERT(entry.is_set);
    ASSERT(!entry.is_valid_path);
}

// --- Real environment variable (TEMP should exist on Windows) ---

static void test_real_env_var() {
    gspl::studio::EnvironmentProvider env({"TEMP"});
    auto entry = env.get("TEMP");
    ASSERT(entry.is_set);
    ASSERT(!entry.value.empty());
}

// --- Provide default allowlist, then get a real var ---

static void test_default_allowlist_real_var() {
    gspl::studio::EnvironmentProvider env(gspl::studio::EnvironmentProvider::default_allowlist());
    bool found_any = false;
    for (const auto& name : {"USERPROFILE", "HOME", "TEMP", "TMP"}) {
        auto entry = env.get(name);
        if (entry.is_set) {
            found_any = true;
            break;
        }
    }
    ASSERT(found_any);
}

// --- Determinism: same provider, same override, same result ---

static void test_deterministic_override() {
    gspl::studio::EnvironmentProvider env({"VAR"});
    env.set_override("VAR", "deterministic");
    auto r1 = env.get("VAR");
    auto r2 = env.get("VAR");
    ASSERT(r1.value == r2.value);
    ASSERT(r1.is_set == r2.is_set);
}

// --- Distinct keys stored separately ---

static void test_distinct_override_keys() {
    gspl::studio::EnvironmentProvider env({"ALPHA", "BETA"});
    env.set_override("ALPHA", "first");
    env.set_override("BETA", "second");
    ASSERT(env.get("ALPHA").value == "first");
    ASSERT(env.get("BETA").value == "second");
    ASSERT(env.get("ALPHA").is_set);
    ASSERT(env.get("BETA").is_set);
}

int main() {
    std::printf("=== Environment Provider Tests ===\n\n");

    TEST(test_default_allowlist_not_empty);
    TEST(test_default_allowlist_contains_home);
    TEST(test_get_unset_variable);
    TEST(test_get_override);
    TEST(test_override_empty_value);
    TEST(test_non_allowlisted_var_blocked);
    TEST(test_clear_overrides);
    TEST(test_set_allowlist);
    TEST(test_get_path_invalid);
    TEST(test_get_path_empty);
    TEST(test_real_env_var);
    TEST(test_default_allowlist_real_var);
    TEST(test_deterministic_override);
    TEST(test_distinct_override_keys);

    std::printf("\n=== Results: %d tests, %d failed ===\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
