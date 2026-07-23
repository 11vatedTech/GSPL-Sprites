#include "gspl/studio/workspace.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace gspl::studio {

struct Workspace::Impl {
    Config config;
    bool open{false};
    std::vector<std::shared_ptr<Project>> projects;

    struct DbStmt;

    // Minimal SQLite wrapper for workspace persistence
    // In production, link against sqlite3 directly

    struct Db {
        std::string path;
        std::fstream file;
        bool connected{false};

        auto connect() -> bool {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
            connected = file.is_open();
            return connected;
        }

        void disconnect() {
            if (file.is_open()) {
                file.close();
            }
            connected = false;
        }

        auto exec(std::string_view sql) -> bool {
            // Placeholder — real SQLite integration would use sqlite3_exec
            // For now, persist schema via flat file alongside DB
            auto schema_path = std::filesystem::path(path).replace_extension(".schema");
            std::ofstream schema_out(schema_path, std::ios::app);
            if (schema_out) {
                schema_out << "-- " << sql << "\n";
            }
            return true;
        }
    } db;

    ~Impl() {
        db.disconnect();
    }
};

Workspace::Workspace(Config config)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = std::move(config);
}

Workspace::~Workspace() = default;

auto Workspace::open() -> bool {
    if (impl_->open) {
        return true;
    }

    auto db_path = impl_->config.workspace_dir / impl_->config.db_filename;
    impl_->db.path = db_path.string();

    if (!impl_->db.connect()) {
        return false;
    }

    // Create workspace directory if needed
    std::filesystem::create_directories(impl_->config.workspace_dir);

    // Initialize schema
    impl_->db.exec(
        "CREATE TABLE IF NOT EXISTS projects ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  path TEXT NOT NULL,"
        "  added_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ")"
    );

    impl_->db.exec(
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ")"
    );

    impl_->db.exec(
        "CREATE TABLE IF NOT EXISTS layout ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  state TEXT NOT NULL"
        ")"
    );

    impl_->db.exec(
        "CREATE TABLE IF NOT EXISTS recent_files ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  path TEXT NOT NULL,"
        "  opened_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ")"
    );

    // Load existing projects from stored paths
    for (const auto& proj_dir : impl_->config.project_dirs) {
        if (Project::is_valid_project_dir(proj_dir)) {
            auto project = std::make_shared<Project>(proj_dir);
            if (project->load()) {
                impl_->projects.push_back(std::move(project));
            }
        }
    }

    impl_->open = true;
    return true;
}

void Workspace::close() {
    impl_->projects.clear();
    impl_->db.disconnect();
    impl_->open = false;
}

auto Workspace::is_open() const -> bool {
    return impl_->open;
}

auto Workspace::add_project(std::filesystem::path project_dir) -> bool {
    if (!Project::is_valid_project_dir(project_dir)) {
        return false;
    }

    auto project = std::make_shared<Project>(project_dir);
    if (!project->load()) {
        return false;
    }

    auto it = std::find_if(impl_->projects.begin(), impl_->projects.end(),
        [&](const auto& p) { return p->root() == project->root(); });
    if (it != impl_->projects.end()) {
        return false;
    }

    impl_->projects.push_back(std::move(project));

    auto abs_path = std::filesystem::absolute(project_dir).string();
    auto stmt = "INSERT OR IGNORE INTO projects (name, path) VALUES ('"
        + project->manifest().name + "', '" + abs_path + "')";
    impl_->db.exec(stmt);

    return true;
}

auto Workspace::remove_project(std::string_view project_name) -> bool {
    auto it = std::find_if(impl_->projects.begin(), impl_->projects.end(),
        [&](const auto& p) { return p->manifest().name == project_name; });
    if (it == impl_->projects.end()) {
        return false;
    }

    auto stmt = std::string("DELETE FROM projects WHERE name = '")
        + std::string(project_name) + "'";
    impl_->db.exec(stmt);

    impl_->projects.erase(it);
    return true;
}

auto Workspace::projects() const -> const std::vector<std::shared_ptr<Project>>& {
    return impl_->projects;
}

auto Workspace::config() const -> const Config& {
    return impl_->config;
}

auto Workspace::create_at(std::filesystem::path dir) -> Workspace {
    std::filesystem::create_directories(dir);

    Workspace ws(Config{
        .workspace_dir = std::move(dir),
        .project_dirs = {},
        .db_filename = "workspace.db"
    });

    ws.open();
    return ws;
}

} // namespace gspl::studio
