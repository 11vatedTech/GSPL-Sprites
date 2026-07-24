#include "gspl/package/package_creator.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <cstdio>

namespace gspl::package {
namespace fs = std::filesystem;

namespace {

bool is_safe_component(std::string_view s) {
    if (s.empty() || s.size() > 128) return false;
    for (auto c : s) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|' || c == '\0')
            return false;
    }
    return s != "." && s != "..";
}

bool is_valid_semver(std::string_view v) {
    static const std::regex re(R"(^(\d+)\.(\d+)\.(\d+)(-[a-zA-Z0-9.]+)?(\+[a-zA-Z0-9.]+)?$)");
    return std::regex_match(v.begin(), v.end(), re);
}

} // anonymous namespace

bool CreatorState::validate_metadata(std::string* error_out) const {
    if (publisher.empty()) { if (error_out) *error_out = "Publisher is required"; return false; }
    if (!is_safe_component(publisher)) { if (error_out) *error_out = "Publisher contains invalid characters"; return false; }
    if (package_name.empty()) { if (error_out) *error_out = "Package name is required"; return false; }
    if (!is_safe_component(package_name)) { if (error_out) *error_out = "Package name contains invalid characters"; return false; }
    if (!is_valid_semver(version)) { if (error_out) *error_out = "Version must be valid semver (e.g. 1.0.0)"; return false; }
    if (display_name.empty()) { if (error_out) *error_out = "Display name is required"; return false; }
    return true;
}

bool CreatorState::validate_dependencies(std::string* error_out) const {
    for (auto const& dep : dependencies) {
        if (dep.package_id.empty()) {
            if (error_out) *error_out = "Dependency package_id is required";
            return false;
        }
        if (dep.package_id.find('/') == std::string_view::npos) {
            if (error_out) *error_out = "Dependency package_id must be in format publisher/name";
            return false;
        }
    }
    return true;
}

struct PackageCreator::Impl {
    fs::path packages_root;
    ProgressCallback progress_cb;

    void progress(std::string_view op, float pct) {
        if (progress_cb) progress_cb(op, pct);
    }
};

PackageCreator::PackageCreator(std::string packages_root)
    : impl_(std::make_unique<Impl>()) {
    impl_->packages_root = fs::absolute(fs::path(packages_root));
}

PackageCreator::~PackageCreator() = default;

void PackageCreator::set_progress_callback(ProgressCallback cb) {
    impl_->progress_cb = std::move(cb);
}

PackageManifest PackageCreator::build_manifest(const CreatorState& state) const {
    PackageManifest mf;
    mf.publisher = state.publisher;
    mf.package_name = state.package_name;
    mf.version = state.version;
    mf.display_name = state.display_name.empty() ? state.package_name : state.display_name;
    mf.description = state.description;
    mf.description_mime_type = state.description_mime_type;
    mf.license = state.license.empty() ? "UNLICENSED" : state.license;
    mf.gspl_version = state.gspl_version;
    mf.authors = state.authors;
    mf.dependencies = state.dependencies;
    mf.tags = state.tags;
    mf.repository = state.repository;
    mf.homepage = state.homepage;
    return mf;
}

std::string PackageCreator::resolve_path(const CreatorState& state) const {
    return (impl_->packages_root / state.publisher / state.package_name / state.version).string();
}

bool PackageCreator::create(const CreatorState& state, std::string* error_out) {
    // Validate
    if (!state.validate_metadata(error_out)) {
        impl_->progress("error", 0);
        return false;
    }
    if (!state.validate_dependencies(error_out)) {
        impl_->progress("error", 0);
        return false;
    }

    impl_->progress("creating", 0.1f);

    fs::path target_dir = impl_->packages_root / state.publisher / state.package_name / state.version;

    try {
        // Create directory structure
        fs::create_directories(target_dir);
        impl_->progress("directories", 0.3f);

        // Write manifest
        auto mf = build_manifest(state);
        {
            std::ofstream of(target_dir / "manifest.json");
            if (!of) {
                if (error_out) *error_out = "Failed to write manifest.json";
                impl_->progress("error", 0);
                return false;
            }
            of << mf.to_json();
        }
        impl_->progress("manifest", 0.5f);

        // Write example source
        if (state.include_example_source) {
            auto source_content = generate_example_source(state);
            std::ofstream of(target_dir / "main.gspl");
            if (of) of << source_content;
        }
        impl_->progress("source", 0.7f);

        // Write README
        if (state.include_readme) {
            auto readme = generate_readme(state);
            std::ofstream of(target_dir / "README.md");
            if (of) of << readme;
        }
        impl_->progress("readme", 0.8f);

        // Create .gspl-ignore placeholder
        {
            std::ofstream of(target_dir / ".gspl-ignore");
            if (of) of << "# Files/directories to exclude from package\n";
        }

        impl_->progress("created", 1.0f);
        return true;

    } catch (std::exception const& e) {
        if (error_out) *error_out = e.what();
        impl_->progress("error", 0);
        return false;
    }
}

std::string PackageCreator::generate_example_source(const CreatorState& state) const {
    std::string s;
    s += "module " + state.publisher + "." + state.package_name + ";\n";
    s += "\n";
    s += "entity " + state.display_name + " {\n";
    s += "    gene identity {\n";
    s += "        stable_id: \"" + state.publisher + "/" + state.package_name + "\";\n";
    s += "        display_name: \"" + state.display_name + "\";\n";
    s += "    }\n";
    s += "\n";
    if (!state.description.empty()) {
        s += "    /// " + state.description + "\n";
    }
    s += "    classification \"entity\";\n";
    s += "\n";
    s += "    morphology {\n";
    s += "        part core { size 1.0, 1.0, 1.0; }\n";
    s += "    }\n";
    s += "}\n";
    return s;
}

std::string PackageCreator::generate_readme(const CreatorState& state) const {
    std::string s;
    s += "# " + state.display_name + "\n";
    s += "\n";
    s += "## Overview\n";
    s += state.description.empty() ? "A GSPL entity package." : state.description;
    s += "\n\n";
    s += "## License\n";
    s += state.license + "\n";
    s += "\n";
    s += "## Dependencies\n";
    if (state.dependencies.empty()) {
        s += "*None*\n";
    } else {
        for (auto const& dep : state.dependencies) {
            s += "- " + dep.package_id;
            if (!dep.version_constraint.empty()) s += " (" + dep.version_constraint + ")";
            s += "\n";
        }
    }
    s += "\n";
    if (!state.authors.empty()) {
        s += "## Authors\n";
        for (auto const& a : state.authors) {
            s += "- " + a + "\n";
        }
    }
    return s;
}

} // namespace gspl::package
