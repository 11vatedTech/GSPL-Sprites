#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gspl {
class ArtifactCache;
}

namespace gspl::studio {

struct ArtifactEntry {
    std::string key;
    std::string content_hash;
    std::uint64_t size = 0;
    std::uint64_t created_at = 0;
    std::vector<std::string> dependency_keys;
    std::string compiler_version;
    std::string provider_identity;
    std::uint64_t entropy_seed = 0;
    bool valid = true;
    std::string validation_error;
};

class ArtifactExplorerModel {
public:
    explicit ArtifactExplorerModel(gspl::ArtifactCache& cache);
    ~ArtifactExplorerModel() = default;

    // Refresh from current cache state
    void refresh();

    // Get all entries
    std::vector<ArtifactEntry> const& entries() const { return entries_; }

    // Get entry by index
    ArtifactEntry const* entry(size_t index) const;

    // Find by key (exact match)
    ArtifactEntry const* find_by_key(std::string_view key) const;

    // Search by content hash prefix
    std::vector<ArtifactEntry const*> search_by_hash(std::string_view hash_prefix) const;

    // Get entries filtered by provider identity
    std::vector<ArtifactEntry const*> filter_by_provider(std::string_view provider) const;

    // Get entries filtered by compiler version
    std::vector<ArtifactEntry const*> filter_by_compiler(std::string_view version) const;

    // Inspect: verify integrity of a single entry
    std::string inspect(size_t index);

    // Validate integrity of all entries
    std::vector<std::pair<size_t, std::string>> validate_all();

    // Invalidate a single entry
    bool invalidate(size_t index);

    // Invalidate all entries
    bool invalidate_all();

    // Prune cache: evict oldest entries until total size is under target_bytes
    std::uint64_t prune(std::uint64_t target_bytes);

    // Trigger integrity verification, return (index, message) for each failed entry
    std::vector<std::pair<size_t, std::string>> verify_integrity();

    // Get total cache statistics
    std::uint64_t total_size() const { return total_size_; }
    std::uint64_t total_entries() const { return entries_.size(); }
    bool is_read_only() const;
    bool is_enabled() const;

    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }

private:
    gspl::ArtifactCache& cache_;
    std::vector<ArtifactEntry> entries_;
    std::uint64_t total_size_ = 0;
    ChangeCallback change_cb_;

    ArtifactEntry from_cache_entry(std::string const& key,
                                   std::string const& content_hash,
                                   std::uint64_t size,
                                   std::uint64_t created_at,
                                   std::vector<std::string> const& deps,
                                   std::string const& compiler_ver,
                                   std::string const& provider_id,
                                   std::uint64_t entropy_seed);
};

} // namespace gspl::studio
