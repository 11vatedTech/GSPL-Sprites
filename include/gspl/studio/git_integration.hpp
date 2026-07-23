#pragma once

#include <string>
#include <vector>
#include <memory>

namespace gspl::studio {

struct GitStatus {
    std::string file_path;
    char index_status{' '};
    char worktree_status{' '};

    [[nodiscard]] bool is_modified() const;
    [[nodiscard]] bool is_added() const;
    [[nodiscard]] bool is_deleted() const;
    [[nodiscard]] bool is_renamed() const;
    [[nodiscard]] bool is_untracked() const;
};

struct GitCommit {
    std::string hash;
    std::string author;
    std::string message;
    std::string timestamp;
};

class GitIntegration {
public:
    explicit GitIntegration(std::string repo_path);
    ~GitIntegration() = default;

    [[nodiscard]] bool is_git_repo() const;
    [[nodiscard]] auto current_branch() const -> std::string;
    [[nodiscard]] auto status() const -> std::vector<GitStatus>;
    [[nodiscard]] auto log(int count = 10) const -> std::vector<GitCommit>;

    bool stage(const std::string& file_path);
    bool unstage(const std::string& file_path);
    bool commit(const std::string& message);
    bool checkout_branch(const std::string& branch);

    [[nodiscard]] std::string diff(const std::string& file_path = "") const;
    [[nodiscard]] std::string diff_staged(const std::string& file_path = "") const;
    [[nodiscard]] std::string blame(const std::string& file_path, int line) const;

private:
    std::string exec_git(const std::string& args) const;
    std::string repo_path_;
};

} // namespace gspl::studio
