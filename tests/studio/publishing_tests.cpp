#include "gspl/studio/publishing.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("assertion failed: " #cond); } while(0)
#define ASSERT_EQ(a, b) do { auto va = (a); auto vb = (b); if (va != vb) throw std::runtime_error(std::string("assertion failed: " #a " == " #b)); } while(0)

using gspl::studio::PublishingManager;
using gspl::studio::PublishTarget;
using gspl::studio::PublishChannel;

static void test_no_targets_by_default() {
    PublishingManager pm;
    ASSERT(pm.targets().empty());
}

static void test_add_target() {
    PublishingManager pm;
    PublishTarget t;
    t.id = "local-reg";
    t.name = "Local";
    t.type = "local-registry";
    t.url = "file:///tmp/reg";
    bool ok = pm.add_target(std::move(t));
    ASSERT(ok);
    ASSERT_EQ(pm.targets().size(), 1u);
}

static void test_add_duplicate_target_fails() {
    PublishingManager pm;
    PublishTarget t1; t1.id = "dup"; t1.name = "Dup"; t1.type = "local-registry";
    PublishTarget t2; t2.id = "dup"; t2.name = "Dup2"; t2.type = "local-registry";
    pm.add_target(std::move(t1));
    bool ok = pm.add_target(std::move(t2));
    ASSERT(!ok);
}

static void test_remove_target() {
    PublishingManager pm;
    PublishTarget t; t.id = "to-go"; t.name = "To Go"; t.type = "local-registry";
    pm.add_target(std::move(t));
    pm.remove_target("to-go");
    ASSERT(pm.targets().empty());
}

static void test_publish_fails_with_no_target() {
    PublishingManager pm;
    auto result = pm.publish("/nonexistent", "missing", PublishChannel::Stable);
    ASSERT(!result.success);
    ASSERT(!result.error_message.empty());
}

static void test_publish_history_empty_for_missing_target() {
    PublishingManager pm;
    auto hist = pm.publish_history("nonexistent");
    ASSERT(hist.empty());
}

static void test_rollback_fails_for_missing_target() {
    PublishingManager pm;
    auto result = pm.rollback("missing", "1.0.0");
    ASSERT(!result.success);
}

int main() {
    TEST(test_no_targets_by_default);
    TEST(test_add_target);
    TEST(test_add_duplicate_target_fails);
    TEST(test_remove_target);
    TEST(test_publish_fails_with_no_target);
    TEST(test_publish_history_empty_for_missing_target);
    TEST(test_rollback_fails_for_missing_target);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
