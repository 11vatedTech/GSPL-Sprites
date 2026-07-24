#include "gspl/studio/project_tree_model.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace gspl::studio {

ProjectTreeModel::ProjectTreeModel(std::filesystem::path project_root)
    : project_root_(std::move(project_root)) {}

void ProjectTreeModel::refresh() {
    root_.name = project_root_.filename().string();
    root_.relative_path = "";
    root_.is_directory = true;
    root_.kind = TreeEntryKind::Directory;
    root_.children.clear();

    if (std::filesystem::exists(project_root_)) {
        build_tree(project_root_, root_);
    }

    flat_.clear();
    flatten(root_, flat_);
}

void ProjectTreeModel::build_tree(std::filesystem::path const& dir, TreeEntry& parent) {
    try {
        for (auto const& entry : std::filesystem::directory_iterator(dir)) {
            TreeEntry child;
            child.name = entry.path().filename().string();
            child.relative_path = std::filesystem::relative(entry.path(), project_root_).generic_string();
            child.is_directory = entry.is_directory();
            child.kind = classify(entry.path());
            child.size = entry.is_regular_file() ? entry.file_size() : 0;

            // Git status
            if (git_status_provider_) {
                auto changed = git_status_provider_();
                child.has_git_changes = std::find(changed.begin(), changed.end(), child.relative_path) != changed.end();
            }

            // Compile error
            if (compile_error_provider_) {
                child.has_compile_error = compile_error_provider_(child.relative_path);
            }

            if (entry.is_directory()) {
                build_tree(entry.path(), child);
            }
            parent.children.push_back(std::move(child));
        }
    } catch (...) {
        // silently skip inaccessible
    }

    std::sort(parent.children.begin(), parent.children.end(),
        [](TreeEntry const& a, TreeEntry const& b) {
            if (a.is_directory != b.is_directory) return a.is_directory;
            return a.name < b.name;
        });
}

void ProjectTreeModel::flatten(TreeEntry const& entry, std::vector<TreeEntry>& out) {
    out.push_back(entry);
    for (auto const& child : entry.children) {
        flatten(child, out);
    }
}

TreeEntryKind ProjectTreeModel::classify(std::filesystem::path const& p) const {
    if (!p.has_extension()) return TreeEntryKind::Other;
    auto ext = p.extension().string();
    if (ext == ".gspl") return TreeEntryKind::SourceFile;
    if (ext == ".jsonc" || ext == ".json") return TreeEntryKind::ConfigFile;
    if (ext == ".gspl-package") return TreeEntryKind::PackageFile;
    return TreeEntryKind::Other;
}

TreeEntry const* ProjectTreeModel::find_by_path(std::string_view relative_path) const {
    for (auto const& e : flat_) {
        if (e.relative_path == relative_path) return &e;
    }
    return nullptr;
}

std::vector<TreeEntry const*> ProjectTreeModel::filter_by_kind(TreeEntryKind kind) const {
    std::vector<TreeEntry const*> result;
    for (auto const& e : flat_) {
        if (e.kind == kind && !e.is_directory) result.push_back(&e);
    }
    return result;
}

bool ProjectTreeModel::create_file(std::string_view relative_path, std::string_view content) {
    auto full = project_root_ / std::filesystem::path(relative_path);
    if (std::filesystem::exists(full)) return false;
    try {
        std::filesystem::create_directories(full.parent_path());
        std::ofstream ofs(full);
        ofs << content;
        refresh();
        if (change_cb_) change_cb_();
        return true;
    } catch (...) { return false; }
}

bool ProjectTreeModel::create_directory(std::string_view relative_path) {
    auto full = project_root_ / std::filesystem::path(relative_path);
    if (std::filesystem::exists(full)) return false;
    try {
        std::filesystem::create_directories(full);
        refresh();
        if (change_cb_) change_cb_();
        return true;
    } catch (...) { return false; }
}

bool ProjectTreeModel::rename(std::string_view old_path, std::string_view new_path) {
    auto old_full = project_root_ / std::filesystem::path(old_path);
    auto new_full = project_root_ / std::filesystem::path(new_path);
    if (!std::filesystem::exists(old_full) || std::filesystem::exists(new_full)) return false;
    try {
        std::filesystem::rename(old_full, new_full);
        refresh();
        if (change_cb_) change_cb_();
        return true;
    } catch (...) { return false; }
}

bool ProjectTreeModel::remove(std::string_view relative_path) {
    auto full = project_root_ / std::filesystem::path(relative_path);
    if (!std::filesystem::exists(full)) return false;
    try {
        std::filesystem::remove_all(full);
        refresh();
        if (change_cb_) change_cb_();
        return true;
    } catch (...) { return false; }
}

std::string ProjectTreeModel::read_file(std::string_view relative_path) const {
    auto full = project_root_ / std::filesystem::path(relative_path);
    if (!std::filesystem::exists(full) || !std::filesystem::is_regular_file(full)) return {};
    try {
        std::ifstream ifs(full);
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    } catch (...) { return {}; }
}

void ProjectTreeModel::set_git_status_provider(std::function<std::vector<std::string>()> provider) {
    git_status_provider_ = std::move(provider);
}

void ProjectTreeModel::set_compile_error_provider(std::function<bool(std::string_view)> provider) {
    compile_error_provider_ = std::move(provider);
}

} // namespace gspl::studio
