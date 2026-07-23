#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace gspl::studio {

enum class PublishChannel {
    Stable,
    Beta,
    Nightly
};

struct PublishTarget {
    std::string id;
    std::string name;
    std::string type; // "local-registry", "github-releases", "spriteforge"
    std::string url;
    std::string api_key_env_var;
};

struct PublishVersion {
    std::string version;
    std::string changelog;
    PublishChannel channel{PublishChannel::Stable};
    std::chrono::system_clock::time_point timestamp;
    std::string signature;
    std::string package_path;
};

struct PublishResult {
    bool success{false};
    std::string target_id;
    std::string version;
    std::string publish_url;
    std::string error_message;
};

class PublishingManager {
public:
    using ProgressCallback = std::function<void(std::string_view stage, float progress)>;

    PublishingManager();
    ~PublishingManager();

    PublishingManager(const PublishingManager&) = delete;
    PublishingManager& operator=(const PublishingManager&) = delete;

    bool add_target(PublishTarget target);
    bool remove_target(const std::string& id);
    [[nodiscard]] auto targets() const -> const std::vector<PublishTarget>&;

    PublishResult publish(const std::string& package_path, const std::string& target_id,
                          PublishChannel channel, const std::string& changelog = "");

    PublishResult rollback(const std::string& target_id, const std::string& version);

    [[nodiscard]] auto publish_history(const std::string& target_id) const -> std::vector<PublishVersion>;

    void set_progress_callback(ProgressCallback cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
