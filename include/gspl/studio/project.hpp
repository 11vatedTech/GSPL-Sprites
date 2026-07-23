#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <filesystem>
#include <chrono>

namespace gspl::studio {

struct ProjectManifest {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> authors;
    std::vector<std::string> dependencies;
    std::string gspl_version;
};

class Project {
public:
    explicit Project(std::filesystem::path root_dir);

    bool load();
    bool save() const;

    [[nodiscard]] auto root() const -> const std::filesystem::path&;
    [[nodiscard]] auto manifest() const -> const ProjectManifest&;
    [[nodiscard]] auto manifest_path() const -> std::filesystem::path;
    [[nodiscard]] auto source_dir() const -> std::filesystem::path;
    [[nodiscard]] auto artifact_dir() const -> std::filesystem::path;
    [[nodiscard]] auto trace_dir() const -> std::filesystem::path;
    [[nodiscard]] auto package_dir() const -> std::filesystem::path;

    void set_manifest(ProjectManifest m);

    [[nodiscard]] static auto is_valid_project_dir(const std::filesystem::path& dir) -> bool;
    [[nodiscard]] static auto create_at(const std::filesystem::path& dir, ProjectManifest m) -> Project;

private:
    std::filesystem::path root_;
    ProjectManifest manifest_;
};

} // namespace gspl::studio
