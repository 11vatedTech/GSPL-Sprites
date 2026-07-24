#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace gspl::studio {

enum class TreeEntryKind {
    Directory,
    SourceFile,
    ArtifactFile,
    PackageFile,
    ConfigFile,
    Other
};

struct TreeEntry {
    std::string name;
    std::string relative_path;
    TreeEntryKind kind = TreeEntryKind::Other;
    bool is_directory = false;
    bool has_git_changes = false;
    bool has_compile_error = false;
    bool is_compiled = false;
    std::uint64_t size = 0;
    std::vector<TreeEntry> children;
};

class ProjectTreeModel {
public:
    explicit ProjectTreeModel(std::filesystem::path project_root);

    void refresh();

    TreeEntry const* root() const { return &root_; }
    std::vector<TreeEntry> const& flat_entries() const { return flat_; }

    TreeEntry const* find_by_path(std::string_view relative_path) const;
    std::vector<TreeEntry const*> filter_by_kind(TreeEntryKind kind) const;

    bool create_file(std::string_view relative_path, std::string_view content = "");
    bool create_directory(std::string_view relative_path);
    bool rename(std::string_view old_path, std::string_view new_path);
    bool remove(std::string_view relative_path);
    std::string read_file(std::string_view relative_path) const;

    void set_git_status_provider(std::function<std::vector<std::string>()> provider);
    void set_compile_error_provider(std::function<bool(std::string_view)> provider);

    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }

private:
    std::filesystem::path project_root_;
    TreeEntry root_;
    std::vector<TreeEntry> flat_;
    std::function<std::vector<std::string>()> git_status_provider_;
    std::function<bool(std::string_view)> compile_error_provider_;
    ChangeCallback change_cb_;

    void build_tree(std::filesystem::path const& dir, TreeEntry& parent);
    void flatten(TreeEntry const& entry, std::vector<TreeEntry>& out);
    TreeEntryKind classify(std::filesystem::path const& p) const;
};

} // namespace gspl::studio
