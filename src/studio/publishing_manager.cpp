#include "gspl/studio/publishing.hpp"
#include "gspl/studio/environment_provider.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <array>
#include <ctime>
#include <random>

namespace gspl::studio {

namespace {

[[nodiscard]] auto read_file(const std::string& path) -> std::string {
    std::ifstream file(path);
    if (!file) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

[[nodiscard]] auto write_file(const std::string& path, std::string_view content) -> bool {
    std::ofstream file(path);
    if (!file) return false;
    file << content;
    return file.good();
}

[[nodiscard]] auto history_file(const std::string& target_id) -> std::string {
    static EnvironmentProvider env(EnvironmentProvider::default_allowlist());
    auto user = env.get_path("USERPROFILE");
    if (!user.is_set) user = env.get_path("HOME");
    if (!user.is_set || !user.is_valid_path) return ".";
    return user.value + "/.gspl/publish_history_" + target_id + ".txt";
}

[[nodiscard]] auto semver_inc(std::string_view version) -> std::string {
    // Increment patch version in semver MAJOR.MINOR.PATCH
    auto dot1 = version.find('.');
    if (dot1 == std::string_view::npos) return "0.0.1";
    auto dot2 = version.find('.', dot1 + 1);
    if (dot2 == std::string_view::npos) return "0.0.1";
    auto patch_str = version.substr(dot2 + 1);
    int patch = 0;
    for (auto c : patch_str) {
        if (c >= '0' && c <= '9') {
            patch = patch * 10 + (c - '0');
        } else {
            break;
        }
    }
    ++patch;
    return std::string(version.substr(0, dot2 + 1)) + std::to_string(patch);
}

[[nodiscard]] auto make_signature(std::string_view package_path) -> std::string {
    // Simplified signature: hash of package path + timestamp
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::hash<std::string_view> hasher;
    auto h = hasher(package_path) ^ static_cast<size_t>(ts);
    constexpr auto hex = "0123456789abcdef";
    std::string sig;
    sig.reserve(16);
    for (int i = 0; i < 16; ++i) {
        sig += hex[(h >> (i * 4)) & 0xF];
    }
    return sig;
}

[[nodiscard]] auto channel_string(PublishChannel ch) -> const char* {
    switch (ch) {
        case PublishChannel::Stable: return "stable";
        case PublishChannel::Beta:   return "beta";
        case PublishChannel::Nightly: return "nightly";
    }
    return "unknown";
}

[[nodiscard]] auto parse_channel(std::string_view s) -> PublishChannel {
    if (s == "beta") return PublishChannel::Beta;
    if (s == "nightly") return PublishChannel::Nightly;
    return PublishChannel::Stable;
}

} // anonymous namespace

struct PublishingManager::Impl {
    std::vector<PublishTarget> targets;
    ProgressCallback callback;

    void report(std::string_view stage, float progress) {
        if (callback) {
            callback(stage, progress);
        }
    }
};

PublishingManager::PublishingManager()
    : impl_(std::make_unique<Impl>())
{
}

PublishingManager::~PublishingManager() = default;

bool PublishingManager::add_target(PublishTarget target) {
    auto existing = std::find_if(impl_->targets.begin(), impl_->targets.end(),
        [&](const auto& t) { return t.id == target.id; });
    if (existing != impl_->targets.end()) {
        return false;
    }
    impl_->targets.push_back(std::move(target));
    return true;
}

bool PublishingManager::remove_target(const std::string& id) {
    auto it = std::find_if(impl_->targets.begin(), impl_->targets.end(),
        [&](const auto& t) { return t.id == id; });
    if (it == impl_->targets.end()) {
        return false;
    }
    impl_->targets.erase(it);
    return true;
}

auto PublishingManager::targets() const -> const std::vector<PublishTarget>& {
    return impl_->targets;
}

auto PublishingManager::publish(const std::string& package_path, const std::string& target_id,
                                 PublishChannel channel, const std::string& changelog) -> PublishResult {
    PublishResult result;
    result.target_id = target_id;

    auto it = std::find_if(impl_->targets.begin(), impl_->targets.end(),
        [&](const auto& t) { return t.id == target_id; });
    if (it == impl_->targets.end()) {
        result.error_message = "Target not found: " + target_id;
        return result;
    }

    impl_->report("Validating package", 0.1f);

    if (!std::filesystem::exists(package_path)) {
        result.error_message = "Package not found: " + package_path;
        return result;
    }

    impl_->report("Reading version history", 0.2f);

    // Determine next version from history
    auto history = publish_history(target_id);
    std::string next_version = "0.0.1";
    if (!history.empty()) {
        next_version = semver_inc(history.front().version);
    }

    impl_->report("Signing package", 0.4f);

    PublishVersion pv;
    pv.version = next_version;
    pv.changelog = changelog.empty() ? ("Release " + next_version) : changelog;
    pv.channel = channel;
    pv.timestamp = std::chrono::system_clock::now();
    pv.signature = make_signature(package_path);
    pv.package_path = package_path;

    result.version = next_version;

    impl_->report("Uploading", 0.6f);

    if (it->type == "local-registry") {
        // Copy package to registry directory
        auto registry_dir = it->url;
        if (!registry_dir.empty()) {
            std::filesystem::create_directories(registry_dir);
            auto dest = std::filesystem::path(registry_dir) / (next_version + ".gspl");
            try {
                std::filesystem::copy_file(package_path, dest,
                    std::filesystem::copy_options::overwrite_existing);
                result.publish_url = dest.string();
            } catch (const std::exception& e) {
                result.error_message = std::string("Copy failed: ") + e.what();
                return result;
            }
        }
    } else if (it->type == "github-releases") {
        result.publish_url = it->url + "/releases/tag/v" + next_version;
    } else if (it->type == "spriteforge") {
        result.publish_url = it->url + "/packages/" + next_version;
    }

    impl_->report("Recording history", 0.8f);

    // Append to history file
    auto hfile = history_file(target_id);
    std::string entry;
    entry += pv.version + "|";
    entry += std::string(channel_string(pv.channel)) + "|";
    entry += pv.signature + "|";
    entry += pv.package_path + "|";
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        pv.timestamp.time_since_epoch()).count();
    entry += std::to_string(ts) + "|";
    // Replace newlines in changelog with escaped form
    for (auto c : pv.changelog) {
        if (c == '\n') entry += "\\n";
        else if (c == '|') entry += "\\p";
        else entry += c;
    }
    entry += "\n";

    auto full_history = read_file(hfile);
    full_history = entry + full_history;
    [[maybe_unused]] auto written = write_file(hfile, full_history);

    impl_->report("Done", 1.0f);
    result.success = true;
    return result;
}

auto PublishingManager::rollback(const std::string& target_id, const std::string& version) -> PublishResult {
    PublishResult result;
    result.target_id = target_id;
    result.version = version;

    auto history = publish_history(target_id);
    if (history.empty()) {
        result.error_message = "No publish history for target: " + target_id;
        return result;
    }

    auto it = std::find_if(history.begin(), history.end(),
        [&](const auto& pv) { return pv.version == version; });
    if (it == history.end()) {
        result.error_message = "Version not found in history: " + version;
        return result;
    }

    // Re-publish the previous version's package
    if (!it->package_path.empty() && std::filesystem::exists(it->package_path)) {
        result = publish(it->package_path, target_id, it->channel,
                         "Rollback to version " + version);
    } else {
        result.error_message = "Package file for version " + version + " not available";
    }

    return result;
}

auto PublishingManager::publish_history(const std::string& target_id) const -> std::vector<PublishVersion> {
    std::vector<PublishVersion> result;
    auto content = read_file(history_file(target_id));
    if (content.empty()) return result;

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        // Parse: version|channel|signature|package_path|timestamp|changelog
        auto parts = std::array<std::string, 6>{};
        size_t idx = 0;
        std::string current;
        for (size_t i = 0; i < line.size() && idx < 6; ++i) {
            char c = line[i];
            if (c == '|' && (i == 0 || line[i - 1] != '\\')) {
                parts[idx++] = current;
                current.clear();
            } else {
                current += c;
            }
        }
        if (idx < 6) parts[idx] = current;
        if (parts[0].empty()) continue;

        PublishVersion pv;
        pv.version = parts[0];
        pv.channel = parse_channel(parts[1]);
        pv.signature = parts[2];
        pv.package_path = parts[3];
        if (!parts[4].empty()) {
            auto ts = std::stoll(parts[4]);
            pv.timestamp = std::chrono::system_clock::from_time_t(
                static_cast<std::time_t>(ts));
        }
        // Unescape changelog
        std::string cl;
        for (size_t i = 0; i < parts[5].size(); ++i) {
            if (parts[5][i] == '\\' && i + 1 < parts[5].size()) {
                if (parts[5][i + 1] == 'n') { cl += '\n'; ++i; }
                else if (parts[5][i + 1] == 'p') { cl += '|'; ++i; }
                else { cl += parts[5][i]; }
            } else {
                cl += parts[5][i];
            }
        }
        pv.changelog = cl;
        result.push_back(std::move(pv));
    }

    return result;
}

void PublishingManager::set_progress_callback(ProgressCallback cb) {
    impl_->callback = std::move(cb);
}

} // namespace gspl::studio
