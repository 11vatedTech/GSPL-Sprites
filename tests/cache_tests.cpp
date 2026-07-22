#include "gspl/cache.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. Cache disabled mode ----
        {
            gspl::CacheConfig cfg;
            cfg.enabled = false;
            gspl::ArtifactCache cache(cfg);
            check(!cache.is_enabled(), "Disabled cache should report not enabled");
            auto result = cache.get("test-key");
            check(!result.has_value(), "Disabled cache should return nothing");
        }

        // ---- 2. Cache key generation ----
        {
            auto k1 = gspl::ArtifactCache::make_key("hello");
            auto k2 = gspl::ArtifactCache::make_key("hello");
            auto k3 = gspl::ArtifactCache::make_key("world");
            check(k1 == k2, "Same content should produce same key");
            check(k1 != k3, "Different content should produce different keys");
        }

        // ---- 3. Hash inputs ----
        {
            auto h1 = gspl::ArtifactCache::hash_inputs({"a", "b"});
            auto h2 = gspl::ArtifactCache::hash_inputs({"a", "b"});
            auto h3 = gspl::ArtifactCache::hash_inputs({"a", "c"});
            check(h1 == h2, "Same inputs should produce same hash");
            check(h1 != h3, "Different inputs should produce different hash");
        }

        // ---- 4. Put and get round-trip ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_4";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.cache_root = tmp;
            cfg.enabled = true;
            {
                gspl::ArtifactCache cache(cfg);
                std::vector<char> data = {'h', 'e', 'l', 'l', 'o'};
                bool ok = cache.put("test-key", data);
                check(ok, "Put should succeed");
                check(cache.contains("test-key"), "Key should exist after put");
            }
            {
                gspl::ArtifactCache cache(cfg);
                auto result = cache.get("test-key");
                check(result.has_value(), "Get should return data");
                check(result->size() == 5, "Data size should be 5");
                check((*result)[0] == 'h', "First byte should be 'h'");
                check((*result)[4] == 'o', "Last byte should be 'o'");
            }
            std::filesystem::remove_all(tmp);
        }

        // ---- 5. Cold miss ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_5";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.cache_root = tmp;
            gspl::ArtifactCache cache(cfg);
            auto result = cache.get("nonexistent");
            check(!result.has_value(), "Non-existent key should miss");
            std::filesystem::remove_all(tmp);
        }

        // ---- 6. Invalidation ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_6";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.cache_root = tmp;
            gspl::ArtifactCache cache(cfg);
            cache.put("key1", std::vector<char>{'a'});
            cache.put("key2", std::vector<char>{'b'});
            check(cache.contains("key1"), "Key1 should exist");
            check(cache.entry_count() == 2, "Should have 2 entries");

            cache.invalidate("key1");
            check(!cache.contains("key1"), "Key1 should be gone after invalidation");
            check(cache.entry_count() == 1, "Should have 1 entry after invalidation");

            cache.invalidate_all();
            check(cache.entry_count() == 0, "All entries should be cleared");
            std::filesystem::remove_all(tmp);
        }

        // ---- 7. Eviction (oldest entry removed) ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_7";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.max_entries = 2;
            cfg.cache_root = tmp;
            gspl::ArtifactCache cache(cfg);
            cache.put("a", std::vector<char>{'1'});
            cache.put("b", std::vector<char>{'2'});
            cache.put("c", std::vector<char>{'3'});
            check(cache.entry_count() <= 2, "Should have at most 2 entries after eviction");
            std::filesystem::remove_all(tmp);
        }

        // ---- 8. Integrity validation ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_8";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.cache_root = tmp;
            gspl::ArtifactCache cache(cfg);
            cache.put("intact", std::vector<char>{'d', 'a', 't', 'a'});
            auto result = cache.validate_integrity();
            check(result.ok(), "Integrity should pass for intact cache");
            std::filesystem::remove_all(tmp);
        }

        // ---- 9. Read-only cache ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cache_test_9";
            std::filesystem::remove_all(tmp);
            gspl::CacheConfig cfg;
            cfg.cache_root = tmp;
            cfg.read_only = true;
            gspl::ArtifactCache cache(cfg);
            bool ok = cache.put("ro-key", std::vector<char>{'x'});
            check(!ok, "Read-only cache should reject puts");
            std::filesystem::remove_all(tmp);
        }

        std::cout << "ALL CACHE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
