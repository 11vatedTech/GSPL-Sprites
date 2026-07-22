#include "gspl/cache.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>

namespace gspl {

ArtifactCache::ArtifactCache(CacheConfig config) : config_(std::move(config)) {
    if (config_.enabled && !config_.cache_root.empty()) {
        init_cache_dir();
    }
}

ArtifactCache::~ArtifactCache() = default;

void ArtifactCache::init_cache_dir() {
    try {
        std::filesystem::create_directories(config_.cache_root);
        initialized_ = true;
        for (auto const& entry : std::filesystem::directory_iterator(config_.cache_root)) {
            if (entry.path().extension() == ".meta") {
                auto key = entry.path().stem().string();
                auto meta = load_meta(key);
                if (!meta.key.empty()) {
                    entries_[key] = meta;
                }
            }
        }
    } catch (...) {
        initialized_ = false;
    }
}

std::filesystem::path ArtifactCache::entry_path(std::string const& key) const {
    return config_.cache_root / (key + ".dat");
}

std::filesystem::path ArtifactCache::meta_path(std::string const& key) const {
    return config_.cache_root / (key + ".meta");
}

CacheEntry ArtifactCache::load_meta(std::string const& key) const {
    CacheEntry entry;
    auto mp = meta_path(key);
    if (!std::filesystem::exists(mp)) return entry;
    std::ifstream f(mp);
    if (!f) return entry;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto k = line.substr(0, eq);
        auto v = line.substr(eq + 1);
        if (k == "key") entry.key = v;
        else if (k == "hash") entry.content_hash = v;
        else if (k == "size") entry.size = std::stoull(v);
        else if (k == "created") entry.created_at = std::stoull(v);
        else if (k == "compiler") entry.compiler_version = v;
        else if (k == "provider") entry.provider_identity = v;
        else if (k == "entropy") entry.entropy_seed = std::stoull(v);
        else if (k == "dep") entry.dependency_keys.push_back(v);
    }
    return entry;
}

void ArtifactCache::save_meta(CacheEntry const& entry) {
    auto mp = meta_path(entry.key);
    std::ofstream f(mp);
    if (!f) return;
    f << "key=" << entry.key << "\n";
    f << "hash=" << entry.content_hash << "\n";
    f << "size=" << entry.size << "\n";
    f << "created=" << entry.created_at << "\n";
    f << "compiler=" << entry.compiler_version << "\n";
    f << "provider=" << entry.provider_identity << "\n";
    f << "entropy=" << entry.entropy_seed << "\n";
    for (auto const& dep : entry.dependency_keys) {
        f << "dep=" << dep << "\n";
    }
}

bool ArtifactCache::is_stale(CacheEntry const& entry) const {
    if (!compiler_version_.empty() && entry.compiler_version != compiler_version_) return true;
    if (!provider_identity_.empty() && entry.provider_identity != provider_identity_) return true;
    return false;
}

std::string ArtifactCache::make_key(std::string const& content) {
    // Simple content-based key using FNV-1a
    std::uint64_t hash = 14695981039346656037ULL;
    for (auto c : content) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ULL;
    }
    std::ostringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string ArtifactCache::hash_inputs(std::vector<std::string> const& inputs) {
    std::string combined;
    for (auto const& inp : inputs) combined += inp + "|";
    return make_key(combined);
}

std::optional<std::vector<char>> ArtifactCache::get(std::string const& key) {
    if (!initialized_ || !config_.enabled) return std::nullopt;
    auto it = entries_.find(key);
    if (it == entries_.end()) return std::nullopt;
    if (is_stale(it->second)) {
        invalidate(key);
        return std::nullopt;
    }
    auto ep = entry_path(key);
    if (!std::filesystem::exists(ep)) {
        entries_.erase(key);
        return std::nullopt;
    }
    std::ifstream f(ep, std::ios::binary);
    if (!f) return std::nullopt;
    auto size = static_cast<std::size_t>(std::filesystem::file_size(ep));
    std::vector<char> data(size);
    f.read(data.data(), static_cast<std::streamsize>(size));
    if (!f) {
        entries_.erase(key);
        return std::nullopt;
    }
    auto actual_hash = make_key(std::string(data.data(), data.size()));
    if (actual_hash != it->second.content_hash) {
        invalidate(key);
        return std::nullopt;
    }
    return data;
}

bool ArtifactCache::put(std::string const& key, std::vector<char> data, std::vector<std::string> deps) {
    if (!initialized_ || config_.read_only || !config_.enabled) return false;
    while (size() + data.size() > config_.max_bytes && entry_count() > 0) {
        if (!evict_one()) break;
    }
    if (entry_count() >= config_.max_entries) {
        if (!evict_one()) return false;
    }
    auto ep = entry_path(key);
    {
        std::ofstream f(ep, std::ios::binary);
        if (!f) return false;
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!f) return false;
    }
    CacheEntry entry;
    entry.key = key;
    entry.content_hash = make_key(std::string(data.data(), data.size()));
    entry.size = data.size();
    entry.created_at = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    entry.compiler_version = compiler_version_;
    entry.provider_identity = provider_identity_;
    entry.dependency_keys = std::move(deps);
    save_meta(entry);
    entries_[key] = entry;
    return true;
}

bool ArtifactCache::contains(std::string const& key) const {
    return entries_.find(key) != entries_.end();
}

bool ArtifactCache::invalidate(std::string const& key) {
    auto ep = entry_path(key);
    auto mp = meta_path(key);
    bool removed = false;
    if (std::filesystem::exists(ep)) { std::filesystem::remove(ep); removed = true; }
    if (std::filesystem::exists(mp)) { std::filesystem::remove(mp); removed = true; }
    entries_.erase(key);
    return removed;
}

bool ArtifactCache::invalidate_all() {
    bool any = false;
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        auto key = it->first;
        ++it;
        if (invalidate(key)) any = true;
    }
    entries_.clear();
    return any;
}

std::uint64_t ArtifactCache::size() const {
    std::uint64_t total = 0;
    for (auto const& [_, entry] : entries_) total += entry.size;
    return total;
}

std::uint64_t ArtifactCache::entry_count() const {
    return entries_.size();
}

bool ArtifactCache::evict_one() {
    if (entries_.empty()) return false;
    auto oldest = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.created_at < oldest->second.created_at) oldest = it;
    }
    return invalidate(oldest->first);
}

DiagnosticResult ArtifactCache::validate_integrity() {
    DiagnosticResult result;
    for (auto const& [key, entry] : entries_) {
        auto ep = entry_path(key);
        if (!std::filesystem::exists(ep)) {
            result.add_error(DiagnosticCode::GSPL_RESOURCE_EXCEEDED,
                             "Cache entry missing data file: " + key, {});
            continue;
        }
        auto actual_size = static_cast<std::uint64_t>(std::filesystem::file_size(ep));
        if (actual_size != entry.size) {
            result.add_error(DiagnosticCode::GSPL_RESOURCE_EXCEEDED,
                             "Cache entry size mismatch for: " + key, {});
        }
    }
    return result;
}

} // namespace gspl
