#include "gspl/studio/artifact_explorer_model.hpp"
#include "gspl/cache.hpp"
#include <algorithm>
#include <sstream>

namespace gspl::studio {

ArtifactExplorerModel::ArtifactExplorerModel(gspl::ArtifactCache& cache) : cache_(cache) {}

void ArtifactExplorerModel::refresh() {
    entries_.clear();
    total_size_ = 0;

    auto cache_entries = cache_.list_entries();
    for (auto const& ce : cache_entries) {
        ArtifactEntry ae;
        ae.key = ce.key;
        ae.content_hash = ce.content_hash;
        ae.size = ce.size;
        ae.created_at = ce.created_at;
        ae.dependency_keys = ce.dependency_keys;
        ae.compiler_version = ce.compiler_version;
        ae.provider_identity = ce.provider_identity;
        ae.entropy_seed = ce.entropy_seed;
        ae.valid = true;
        entries_.push_back(std::move(ae));
        total_size_ += ce.size;
    }
}

ArtifactEntry const* ArtifactExplorerModel::entry(size_t index) const {
    if (index >= entries_.size()) return nullptr;
    return &entries_[index];
}

ArtifactEntry const* ArtifactExplorerModel::find_by_key(std::string_view key) const {
    for (auto const& e : entries_) {
        if (e.key == key) return &e;
    }
    return nullptr;
}

std::vector<ArtifactEntry const*> ArtifactExplorerModel::search_by_hash(std::string_view prefix) const {
    std::vector<ArtifactEntry const*> result;
    for (auto const& e : entries_) {
        if (e.content_hash.find(prefix) == 0) result.push_back(&e);
    }
    return result;
}

std::vector<ArtifactEntry const*> ArtifactExplorerModel::filter_by_provider(std::string_view provider) const {
    std::vector<ArtifactEntry const*> result;
    for (auto const& e : entries_) {
        if (e.provider_identity == provider) result.push_back(&e);
    }
    return result;
}

std::vector<ArtifactEntry const*> ArtifactExplorerModel::filter_by_compiler(std::string_view version) const {
    std::vector<ArtifactEntry const*> result;
    for (auto const& e : entries_) {
        if (e.compiler_version == version) result.push_back(&e);
    }
    return result;
}

std::string ArtifactExplorerModel::inspect(size_t index) {
    if (index >= entries_.size()) return "error: index out of range";
    auto& entry = entries_[index];

    auto data = cache_.get(entry.key);
    if (!data) return "error: artifact not found in cache";

    std::ostringstream oss;
    oss << "Key: " << entry.key << "\n";
    oss << "Hash: " << entry.content_hash << "\n";
    oss << "Size: " << entry.size << " bytes\n";
    oss << "Created: " << entry.created_at << "\n";
    oss << "Compiler: " << entry.compiler_version << "\n";
    oss << "Provider: " << entry.provider_identity << "\n";
    oss << "Entropy: " << entry.entropy_seed << "\n";
    oss << "Dependencies: " << entry.dependency_keys.size() << "\n";
    for (auto const& dep : entry.dependency_keys) {
        oss << "  - " << dep << "\n";
    }
    return oss.str();
}

std::vector<std::pair<size_t, std::string>> ArtifactExplorerModel::validate_all() {
    std::vector<std::pair<size_t, std::string>> results;
    auto dr = cache_.validate_integrity();
    for (auto const& d : dr.diagnostics) {
        results.push_back({0, d.message});
    }
    for (auto& e : entries_) {
        e.valid = true;
        e.validation_error.clear();
    }
    if (change_cb_) change_cb_();
    return results;
}

bool ArtifactExplorerModel::invalidate(size_t index) {
    if (index >= entries_.size()) return false;
    bool ok = cache_.invalidate(entries_[index].key);
    if (ok) {
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
        if (change_cb_) change_cb_();
    }
    return ok;
}

bool ArtifactExplorerModel::invalidate_all() {
    bool ok = cache_.invalidate_all();
    if (ok) {
        entries_.clear();
        total_size_ = 0;
        if (change_cb_) change_cb_();
    }
    return ok;
}

std::uint64_t ArtifactExplorerModel::prune(std::uint64_t target_bytes) {
    std::uint64_t evicted = 0;
    // Sort entries by created_at ascending (oldest first)
    std::vector<size_t> indices(entries_.size());
    for (size_t i = 0; i < entries_.size(); ++i) indices[i] = i;
    std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        return entries_[a].created_at < entries_[b].created_at;
    });
    while (total_size_ > target_bytes && !indices.empty()) {
        auto idx = indices.back(); indices.pop_back();
        if (cache_.invalidate(entries_[idx].key)) {
            total_size_ -= entries_[idx].size;
            evicted += entries_[idx].size;
            entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(idx));
        }
    }
    if (evicted > 0 && change_cb_) change_cb_();
    return evicted;
}

std::vector<std::pair<size_t, std::string>> ArtifactExplorerModel::verify_integrity() {
    std::vector<std::pair<size_t, std::string>> results;
    for (size_t i = 0; i < entries_.size(); ++i) {
        auto data = cache_.get(entries_[i].key);
        if (!data) {
            entries_[i].valid = false;
            entries_[i].validation_error = "data missing or corrupted";
            results.push_back({i, entries_[i].validation_error});
        } else {
            entries_[i].valid = true;
            entries_[i].validation_error.clear();
        }
    }
    if (!results.empty() && change_cb_) change_cb_();
    return results;
}

bool ArtifactExplorerModel::is_read_only() const {
    return cache_.is_read_only();
}

bool ArtifactExplorerModel::is_enabled() const {
    return cache_.is_enabled();
}

} // namespace gspl::studio
