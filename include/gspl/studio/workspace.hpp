#pragma once

#include "project.hpp"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace gspl::studio {

// Manages workspace state (open projects, window layout, settings)
// Backed by SQLite database stored in the workspace directory
class Workspace {
public:
    struct Config {
        std::filesystem::path workspace_dir;
        std::vector<std::filesystem::path> project_dirs;
        std::string db_filename{"workspace.db"};
    };

    explicit Workspace(Config config);
    ~Workspace();

    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;

    bool open();
    void close();
    [[nodiscard]] bool is_open() const;

    bool add_project(std::filesystem::path project_dir);
    bool remove_project(std::string_view project_name);
    [[nodiscard]] auto projects() const -> const std::vector<std::shared_ptr<Project>>&;

    [[nodiscard]] auto config() const -> const Config&;

    static auto create_at(std::filesystem::path dir) -> Workspace;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
