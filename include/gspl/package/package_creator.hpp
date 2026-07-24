#pragma once

#include "manifest.hpp"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace gspl::package {

enum class CreatorStep {
    Metadata,
    Dependencies,
    Tags,
    License,
    Review,
};

struct CreatorState {
    std::string publisher;
    std::string package_name;
    std::string version = "0.1.0";
    std::string display_name;
    std::string description;
    std::string description_mime_type = "text/markdown";
    std::string license = "UNLICENSED";
    std::string gspl_version = "0.1.0";
    std::string repository;
    std::string homepage;
    std::vector<std::string> authors;
    std::vector<PackageDependency> dependencies;
    std::vector<std::string> tags;
    bool include_example_source = true;
    bool include_readme = true;
    bool sign_package = false;

    bool validate_metadata(std::string* error_out = nullptr) const;
    bool validate_dependencies(std::string* error_out = nullptr) const;
};

class PackageCreator {
public:
    using ProgressCallback = std::function<void(std::string_view op, float pct)>;

    PackageCreator(std::string packages_root);
    ~PackageCreator();

    PackageCreator(const PackageCreator&) = delete;
    PackageCreator& operator=(const PackageCreator&) = delete;

    void set_progress_callback(ProgressCallback cb);

    // Build manifest from current state
    [[nodiscard]] PackageManifest build_manifest(const CreatorState& state) const;

    // Create the package directory structure
    [[nodiscard]] bool create(const CreatorState& state, std::string* error_out = nullptr);

    // Create example source file content for the package
    [[nodiscard]] std::string generate_example_source(const CreatorState& state) const;

    // Create README content for the package
    [[nodiscard]] std::string generate_readme(const CreatorState& state) const;

    // Resolve the output path without creating
    [[nodiscard]] std::string resolve_path(const CreatorState& state) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::package
