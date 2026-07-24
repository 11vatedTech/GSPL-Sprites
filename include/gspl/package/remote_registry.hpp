#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <optional>

namespace gspl::package {

struct RegistryPackage {
    std::string package_id;
    std::string version;
    std::string download_url;
    std::string checksum;
    size_t size = 0;
};

enum class RegistryError {
    None,
    NotFound,
    NetworkError,
    AuthError,
    InvalidResponse,
    NotImplemented,
};

class RemoteRegistry {
public:
    using ProgressCallback = std::function<void(std::string_view op, float pct)>;

    explicit RemoteRegistry(std::string registry_url);
    ~RemoteRegistry();

    RemoteRegistry(const RemoteRegistry&) = delete;
    RemoteRegistry& operator=(const RemoteRegistry&) = delete;

    void set_progress_callback(ProgressCallback cb);

    // Query available versions of a package
    std::optional<std::vector<RegistryPackage>> list_versions(const std::string& package_id);

    // Fetch package manifest by ID and version
    std::optional<RegistryPackage> fetch_package_info(const std::string& package_id, const std::string& version);

    // Download package archive to local path
    std::pair<bool, RegistryError> download(const std::string& package_id, const std::string& version,
                                            const std::string& dest_path);

    // Publish a package to the registry
    std::pair<bool, RegistryError> publish(const std::string& package_id, const std::string& version,
                                           const std::string& archive_path);

    bool is_available() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::package
