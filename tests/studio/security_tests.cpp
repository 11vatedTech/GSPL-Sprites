#include "gspl/studio/authority_model.hpp"
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

using gspl::studio::AuthorityLevel;
using gspl::studio::AuthorityModel;

static void test_default_none() {
    AuthorityModel model;
    ASSERT_EQ(model.authorities().size(), 0u);
}

static void test_workspace_root_gives_admin() {
    auto tmp = fs::temp_directory_path() / "gspl-test-auth-ws";
    fs::create_directories(tmp);
    AuthorityModel model(tmp.string());
    ASSERT_EQ(model.authorities().size(), 1u);
    ASSERT_EQ(model.check_access(tmp.string()), AuthorityLevel::Admin);
    fs::remove_all(tmp);
}

static void test_grant_and_check() {
    AuthorityModel model;
    model.grant_access("/tmp/test", AuthorityLevel::Write);
    ASSERT_EQ(model.check_access("/tmp/test"), AuthorityLevel::Write);
}

static void test_revoke_access() {
    AuthorityModel model;
    model.grant_access("/tmp/test", AuthorityLevel::Write);
    model.revoke_access("/tmp/test");
    ASSERT_EQ(model.check_access("/tmp/test"), AuthorityLevel::None);
}

static void test_default_is_none_for_outside() {
    auto tmp = fs::temp_directory_path() / "gspl-test-auth-out";
    fs::create_directories(tmp);
    AuthorityModel model(tmp.string());
    auto outside = fs::temp_directory_path() / "gspl-test-outside";
    ASSERT_EQ(model.check_access(outside.string()), AuthorityLevel::None);
    fs::remove_all(tmp);
}

static void test_reset_to_defaults_clears_all() {
    AuthorityModel model;
    model.grant_access("/tmp/a", AuthorityLevel::Read);
    model.grant_access("/tmp/b", AuthorityLevel::Write);
    model.reset_to_defaults();
    ASSERT(model.authorities().empty());
}

static void test_set_workspace_root() {
    AuthorityModel model;
    model.set_workspace_root("/test/ws");
    ASSERT_EQ(model.workspace_root(), "/test/ws");
    ASSERT_EQ(model.authorities().size(), 1u);
}

int main() {
    TEST(test_default_none);
    TEST(test_workspace_root_gives_admin);
    TEST(test_grant_and_check);
    TEST(test_revoke_access);
    TEST(test_default_is_none_for_outside);
    TEST(test_reset_to_defaults_clears_all);
    TEST(test_set_workspace_root);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
