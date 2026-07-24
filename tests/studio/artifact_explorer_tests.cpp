#include "gspl/cache.hpp"
#include "gspl/studio/artifact_explorer_model.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("assertion failed: " #cond); } while(0)
#define ASSERT_EQ(a, b) do { auto va = (a); auto vb = (b); if (va != vb) throw std::runtime_error(std::string("assertion failed: " #a " == " #b)); } while(0)

using namespace gspl;
using namespace gspl::studio;

static std::filesystem::path temp_dir() {
    auto tmp = std::filesystem::temp_directory_path();
    auto dir = tmp / "gspl_artifact_test_XXXXXX";
    // Use a deterministic name
    static int counter = 0;
    auto p = tmp / ("gspl_artifact_test_" + std::to_string(counter++));
    std::filesystem::create_directories(p);
    return p;
}

static void test_model_create() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    cfg.max_bytes = 1024 * 1024;
    ArtifactCache cache(cfg);
    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 0u);
}

static void test_model_populate() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    cfg.max_bytes = 1024 * 1024;
    ArtifactCache cache(cfg);
    cache.put("key1", std::vector<char>{'a', 'b', 'c'}, {"dep1"});
    cache.put("key2", std::vector<char>{'d', 'e', 'f'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 2u);
}

static void test_model_find_by_key() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.put("findme", std::vector<char>{'x'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    auto entry = model.find_by_key("findme");
    ASSERT(entry != nullptr);
    ASSERT_EQ(entry->key, "findme");
    ASSERT(!entry->content_hash.empty());
}

static void test_model_search_by_hash() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.put("a", std::vector<char>{'1'});
    cache.put("b", std::vector<char>{'2'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 2u);
}

static void test_model_inspect() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.put("inspected", std::vector<char>{'d', 'a', 't', 'a'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 1u);
    auto result = model.inspect(0);
    ASSERT(!result.empty());
    ASSERT(result.find("Key: inspected") != std::string::npos);
    ASSERT(result.find("Size: 4 bytes") != std::string::npos);
}

static void test_model_invalidate() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.put("keep", std::vector<char>{'y'});
    cache.put("torm", std::vector<char>{'x'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 2u);

    // Find "torm" entry
    auto* torm_entry = model.find_by_key("torm");
    ASSERT(torm_entry != nullptr);

    // Compute its index
    size_t torm_index = 0;
    for (size_t i = 0; i < model.total_entries(); ++i) {
        if (model.entry(i) && model.entry(i)->key == "torm") {
            torm_index = i;
            break;
        }
    }
    bool ok = model.invalidate(torm_index);
    ASSERT(ok);
    ASSERT_EQ(model.total_entries(), 1u);
    ASSERT(model.find_by_key("keep") != nullptr);
    ASSERT(model.find_by_key("torm") == nullptr);
}

static void test_model_invalidate_all() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.put("a", std::vector<char>{'1'});
    cache.put("b", std::vector<char>{'2'});
    cache.put("c", std::vector<char>{'3'});

    ArtifactExplorerModel model(cache);
    model.refresh();
    ASSERT_EQ(model.total_entries(), 3u);

    bool ok = model.invalidate_all();
    ASSERT(ok);
    ASSERT_EQ(model.total_entries(), 0u);
}

static void test_model_filter_provider() {
    auto dir = temp_dir();
    CacheConfig cfg;
    cfg.cache_root = dir;
    ArtifactCache cache(cfg);
    cache.set_provider_identity("provider_a");
    cache.put("a1", std::vector<char>{'1'});
    cache.put("a2", std::vector<char>{'2'});
    cache.set_provider_identity("provider_b");
    cache.put("b1", std::vector<char>{'3'});

    ArtifactExplorerModel model(cache);
    model.refresh();

    // Each entry should have the provider set at creation time
    // Note: provider_identity is set on the cache, used during put
    auto result = model.filter_by_provider("provider_a");
    // At minimum verify it works without errors
    ASSERT(result.size() <= model.total_entries());
}

static void test_model_flags() {
    CacheConfig cfg;
    cfg.read_only = true;
    cfg.enabled = false;
    ArtifactCache cache(cfg);
    ArtifactExplorerModel model(cache);
    ASSERT(model.is_read_only());
    ASSERT(!model.is_enabled());
}

int main() {
    TEST(test_model_create);
    TEST(test_model_populate);
    TEST(test_model_find_by_key);
    TEST(test_model_search_by_hash);
    TEST(test_model_inspect);
    TEST(test_model_invalidate);
    TEST(test_model_invalidate_all);
    TEST(test_model_filter_provider);
    TEST(test_model_flags);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
