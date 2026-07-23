#pragma once

#include "manifest.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace gspl::package {

struct InstalledPackage {
    PackageManifest manifest;
    std::string install_path;
    std::chrono::system_clock::time_point installed_at;
    std::string source_registry; // "local", "remote"
};

enum class PackageOperation {
    Install,
    Update,
    Remove
};

class PackageManager {
public:
    using ProgressCallback = std::function<void(std::string_view operation, float progress)>;

    explicit PackageManager(std::string packages_root);
    ~PackageManager();

    PackageManager(const PackageManager&) = delete;
    PackageManager& operator=(const PackageManager&) = delete;

    bool install(const std::string& package_id, const std::string& version = "");
    bool uninstall(const std::string& package_id);
    bool update(const std::string& package_id, const std::string& new_version = "");

    [[nodiscard]] auto installed_packages() const -> const std::vector<InstalledPackage>&;
    [[nodiscard]] auto find(const std::string& package_id) -> InstalledPackage*;

    [[nodiscard]] bool has_package(const std::string& package_id) const;
    [[nodiscard]] std::vector<std::string> resolve_dependencies(const std::string& package_id) const;

    void set_progress_callback(ProgressCallback cb);
    void set_registry_url(const std::string& url);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::package
