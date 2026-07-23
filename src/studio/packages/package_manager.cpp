#include "gspl/package/package_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <regex>

namespace gspl::package {
namespace fs = std::filesystem;

namespace {

bool is_valid_semver(std::string_view v) {
    static const std::regex semver_re(R"(^(\d+)\.(\d+)\.(\d+)(-[a-zA-Z0-9.]+)?(\+[a-zA-Z0-9.]+)?$)");
    return std::regex_match(v.begin(), v.end(), semver_re);
}

bool is_safe_path_component(std::string_view s) {
    if (s.empty() || s.size() > 128) return false;
    for (auto c : s) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            return false;
        if (static_cast<unsigned char>(c) < 0x20) return false;
    }
    return s != "." && s != "..";
}

} // anonymous namespace

struct PackageManager::Impl {
    fs::path packages_root;
    std::vector<InstalledPackage> installed;
    ProgressCallback progress_cb;
    std::string registry_url;

    void progress(std::string_view op, float pct) {
        if (progress_cb) progress_cb(op, pct);
    }

    void scan_installed() {
        installed.clear();
        if (!fs::exists(packages_root) || !fs::is_directory(packages_root)) return;

        for (auto const& publisher_dir : fs::directory_iterator(packages_root)) {
            if (!publisher_dir.is_directory()) continue;
            std::string publisher = publisher_dir.path().filename().string();
            if (!is_safe_path_component(publisher)) continue;

            for (auto const& package_dir : fs::directory_iterator(publisher_dir.path())) {
                if (!package_dir.is_directory()) continue;
                std::string package_name = package_dir.path().filename().string();
                if (!is_safe_path_component(package_name)) continue;

                // Scan version directories
                for (auto const& version_dir : fs::directory_iterator(package_dir.path())) {
                    if (!version_dir.is_directory()) continue;
                    std::string version = version_dir.path().filename().string();

                    fs::path manifest_path = version_dir.path() / "manifest.json";
                    auto mf = PackageManifest::load(manifest_path.string());
                    if (!mf) continue;

                    // Get installed_at timestamp from directory creation time
                    auto ftime = fs::last_write_time(version_dir.path());
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());

                    InstalledPackage ip;
                    ip.manifest = std::move(*mf);
                    ip.install_path = version_dir.path().string();
                    ip.installed_at = sctp;
                    ip.source_registry = "local";
                    installed.push_back(std::move(ip));
                }
            }
        }

        // Sort by install path for deterministic ordering
        std::sort(installed.begin(), installed.end(),
            [](const InstalledPackage& a, const InstalledPackage& b) {
                return a.install_path < b.install_path;
            });
    }

    InstalledPackage* find_by_id(const std::string& pkg_id) {
        for (auto& ip : installed) {
            if (ip.manifest.id() == pkg_id) return &ip;
        }
        return nullptr;
    }

    // DFS-based dependency resolution with cycle detection
    bool resolve_deps(const std::string& pkg_id,
                      std::vector<std::string>& result,
                      std::set<std::string>& visiting,
                      std::set<std::string>& visited) const {
        if (visited.contains(pkg_id)) return true;
        if (visiting.contains(pkg_id)) {
            // Cycle detected
            return false;
        }

        // Find this package's manifest
        const PackageManifest* mf = nullptr;
        for (auto const& ip : installed) {
            if (ip.manifest.id() == pkg_id) {
                mf = &ip.manifest;
                break;
            }
        }
        if (!mf) return false;

        visiting.insert(pkg_id);

        for (auto const& dep : mf->dependencies) {
            if (!resolve_deps(dep.package_id, result, visiting, visited)) {
                return false;
            }
        }

        visiting.erase(pkg_id);
        visited.insert(pkg_id);
        result.push_back(pkg_id);
        return true;
    }
};

PackageManager::PackageManager(std::string packages_root)
    : impl_(std::make_unique<Impl>()) {
    impl_->packages_root = fs::absolute(fs::path(packages_root));
    impl_->scan_installed();
}

PackageManager::~PackageManager() = default;

bool PackageManager::install(const std::string& package_id, const std::string& version) {
    // Parse publisher/package-name
    auto slash_pos = package_id.find('/');
    if (slash_pos == std::string_view::npos) {
        impl_->progress("error", 0);
        return false;
    }
    std::string publisher = package_id.substr(0, slash_pos);
    std::string pkg_name = package_id.substr(slash_pos + 1);

    if (!is_safe_path_component(publisher) || !is_safe_path_component(pkg_name)) {
        impl_->progress("error", 0);
        return false;
    }

    std::string ver = version;
    if (ver.empty()) {
        // Default to latest: 0.1.0
        ver = "0.1.0";
    }
    if (!is_valid_semver(ver)) {
        impl_->progress("error", 0);
        return false;
    }

    // Check if already installed
    if (impl_->find_by_id(package_id)) {
        impl_->progress("skipped", 1.0f);
        return true;
    }

    impl_->progress("installing", 0.1f);

    // Build target path: packages_root/<publisher>/<package-name>/<version>/
    fs::path target_dir = impl_->packages_root / publisher / pkg_name / ver;

    try {
        fs::create_directories(target_dir);

        // Create a basic manifest file
        PackageManifest mf;
        mf.publisher = publisher;
        mf.package_name = pkg_name;
        mf.version = ver;
        mf.display_name = pkg_name;
        mf.description = "Installed package";
        mf.gspl_version = "0.1.0";
        mf.license = "UNLICENSED";

        fs::path manifest_path = target_dir / "manifest.json";
        {
            std::ofstream of(manifest_path);
            if (!of) {
                impl_->progress("error", 0);
                return false;
            }
            of << mf.to_json();
        }

        impl_->progress("installed", 1.0f);
    } catch (std::exception const&) {
        impl_->progress("error", 0);
        return false;
    }

    // Re-scan installed packages
    impl_->scan_installed();
    return true;
}

bool PackageManager::uninstall(const std::string& package_id) {
    auto* ip = impl_->find_by_id(package_id);
    if (!ip) return false;

    // Check that no other installed package depends on this one
    for (auto const& other : impl_->installed) {
        if (other.manifest.id() == package_id) continue;
        for (auto const& dep : other.manifest.dependencies) {
            if (dep.package_id == package_id) {
                impl_->progress("dependency_blocked", 0);
                return false;
            }
        }
    }

    impl_->progress("uninstalling", 0.5f);

    try {
        fs::path pkg_dir(ip->install_path);
        if (fs::exists(pkg_dir)) {
            fs::remove_all(pkg_dir);
        }
    } catch (std::exception const&) {
        impl_->progress("error", 0);
        return false;
    }

    impl_->progress("uninstalled", 1.0f);
    impl_->scan_installed();
    return true;
}

bool PackageManager::update(const std::string& package_id, const std::string& new_version) {
    auto* ip = impl_->find_by_id(package_id);
    if (!ip) return false;

    std::string target_ver = new_version;
    if (target_ver.empty()) {
        // No version specified; simulate "latest" by incrementing patch
        // In a real implementation this would query the registry
        target_ver = ip->manifest.version;
    }

    if (!is_valid_semver(target_ver)) {
        impl_->progress("error", 0);
        return false;
    }

    // Uninstall old version
    if (!uninstall(package_id)) return false;

    // Install new version
    if (!install(package_id, target_ver)) return false;

    return true;
}

auto PackageManager::installed_packages() const -> const std::vector<InstalledPackage>& {
    return impl_->installed;
}

auto PackageManager::find(const std::string& package_id) -> InstalledPackage* {
    return impl_->find_by_id(package_id);
}

bool PackageManager::has_package(const std::string& package_id) const {
    for (auto const& ip : impl_->installed) {
        if (ip.manifest.id() == package_id) return true;
    }
    return false;
}

std::vector<std::string> PackageManager::resolve_dependencies(const std::string& package_id) const {
    std::vector<std::string> result;
    std::set<std::string> visiting;
    std::set<std::string> visited;

    impl_->resolve_deps(package_id, result, visiting, visited);
    return result;
}

void PackageManager::set_progress_callback(ProgressCallback cb) {
    impl_->progress_cb = std::move(cb);
}

void PackageManager::set_registry_url(const std::string& url) {
    impl_->registry_url = url;
}

} // namespace gspl::package
