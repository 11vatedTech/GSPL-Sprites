#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace gspl::package {

struct PackageDependency {
    std::string package_id;    // publisher/package-name
    std::string version_constraint;
};

struct PackageManifest {
    std::string publisher;
    std::string package_name;
    std::string version;       // semver
    std::string display_name;
    std::string description;
    std::vector<std::string> authors;
    std::string license;
    std::string gspl_version;
    std::string description_mime_type{"text/markdown"};
    std::vector<PackageDependency> dependencies;
    std::vector<std::string> tags;
    std::string repository;
    std::string homepage;
    std::string signature;
    
    [[nodiscard]] std::string id() const; // "publisher/package-name"
    [[nodiscard]] bool validate() const;
    static auto load(const std::string& path) -> std::optional<PackageManifest>;
    [[nodiscard]] std::string to_json() const;
    [[nodiscard]] bool has_signature() const;
};

} // namespace gspl::package
