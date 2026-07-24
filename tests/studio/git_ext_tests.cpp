#include "gspl/studio/git_integration.hpp"
#include "gspl/studio/artifact_explorer_model.hpp"
#include "gspl/cache.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include <fstream>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; \
  fprintf(stdout, "  " name "..."); fflush(stdout); } while(0)
#define PASS() fprintf(stdout, " PASS\n")
#define FAIL(msg) do { fprintf(stdout, " FAIL: %s\n", msg); ++tests_failed; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

// ---- Git extension tests ----
static void test_git_list_branches() {
    TEST("list_branches");
    // Use this repo as the test fixture
    auto repo = std::filesystem::current_path().string();
    gspl::studio::GitIntegration git(repo);
    if (!git.is_git_repo()) {
        fprintf(stdout, " SKIP (not in git repo)\n");
        return;
    }
    auto branches = git.list_branches();
    ASSERT(!branches.empty(), "at least one branch");
    bool found_main = false;
    for (auto const& b : branches) {
        if (b == "main") { found_main = true; break; }
    }
    ASSERT(found_main, "main branch exists");
    PASS();
}

static void test_git_create_delete_branch() {
    TEST("create_branch and delete_branch");
    auto repo = std::filesystem::current_path().string();
    gspl::studio::GitIntegration git(repo);
    if (!git.is_git_repo()) {
        fprintf(stdout, " SKIP (not in git repo)\n");
        return;
    }
    // Create branch
    ASSERT(git.create_branch("__test_temp_branch__") == true, "create branch");
    auto branches = git.list_branches();
    bool found = false;
    for (auto const& b : branches) {
        if (b == "__test_temp_branch__") { found = true; break; }
    }
    ASSERT(found, "branch listed after create");

    // Delete branch
    ASSERT(git.delete_branch("__test_temp_branch__") == true, "delete branch");
    branches = git.list_branches();
    found = false;
    for (auto const& b : branches) {
        if (b == "__test_temp_branch__") { found = true; break; }
    }
    ASSERT(!found, "branch gone after delete");
    PASS();
}

// ---- Cache prune/verify tests ----
static void test_cache_prune() {
    TEST("prune evicts oldest entries");
    auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_prune_test";
    std::filesystem::remove_all(tmp);
    gspl::CacheConfig cfg;
    cfg.cache_root = tmp;
    cfg.max_bytes = 1'000'000;
    cfg.max_entries = 100;
    gspl::ArtifactCache cache(cfg);

    cache.put("key_a", std::vector<char>(100, 'a'), {});
    cache.put("key_b", std::vector<char>(200, 'b'), {});
    cache.put("key_c", std::vector<char>(300, 'c'), {});

    gspl::studio::ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT(model.total_entries() == 3, "3 entries initially");

    auto evicted = model.prune(350);
    ASSERT(evicted > 0, "some data evicted");
    ASSERT(model.total_entries() < 3, "fewer entries after prune");

    std::filesystem::remove_all(tmp);
    PASS();
}

static void test_cache_verify_integrity() {
    TEST("verify_integrity");
    auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_verify_test";
    std::filesystem::remove_all(tmp);
    gspl::CacheConfig cfg;
    cfg.cache_root = tmp;
    gspl::ArtifactCache cache(cfg);
    cache.put("test_key", std::vector<char>(50, 'x'), {});

    gspl::studio::ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT(model.total_entries() == 1, "1 entry");

    auto results = model.verify_integrity();
    ASSERT(results.empty(), "all entries pass integrity");

    std::filesystem::remove_all(tmp);
    PASS();
}

int main() {
    test_git_list_branches();
    test_git_create_delete_branch();
    test_cache_prune();
    test_cache_verify_integrity();

    fprintf(stdout, "\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
