#pragma once
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gspl {

struct CacheConfig {
    std::filesystem::path cache_root;
    std::uint64_t max_bytes{268'435'456};
    std::uint64_t max_entries{4096};
    bool read_only{false};
    bool enabled{true};
};

struct CacheEntry {
    std::string key;
    std::string content_hash;
    std::uint64_t size{};
    std::uint64_t created_at{};
    std::vector<std::string> dependency_keys;
    std::string compiler_version;
    std::string provider_identity;
    std::uint64_t entropy_seed{};
};

class ArtifactCache {
public:
    explicit ArtifactCache(CacheConfig config = {});
    ~ArtifactCache();

    std::optional<std::vector<char>> get(std::string const& key);
    bool put(std::string const& key, std::vector<char> data, std::vector<std::string> deps = {});
    bool contains(std::string const& key) const;
    bool invalidate(std::string const& key);
    bool invalidate_all();
    std::uint64_t size() const;
    std::uint64_t entry_count() const;
    bool evict_one();
    DiagnosticResult validate_integrity();
    bool is_read_only() const { return config_.read_only; }
    bool is_enabled() const { return config_.enabled; }
    void set_compiler_version(std::string v) { compiler_version_ = std::move(v); }
    void set_provider_identity(std::string id) { provider_identity_ = std::move(id); }

    static std::string make_key(std::string const& content);
    static std::string hash_inputs(std::vector<std::string> const& inputs);

private:
    CacheConfig config_;
    std::string compiler_version_;
    std::string provider_identity_;
    std::unordered_map<std::string, CacheEntry> entries_;
    bool initialized_{};

    std::filesystem::path entry_path(std::string const& key) const;
    std::filesystem::path meta_path(std::string const& key) const;
    CacheEntry load_meta(std::string const& key) const;
    void save_meta(CacheEntry const& entry);
    bool is_stale(CacheEntry const& entry) const;
    void init_cache_dir();
};

} // namespace gspl
