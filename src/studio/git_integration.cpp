#include "gspl/studio/git_integration.hpp"
#include <cstdio>
#include <array>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace gspl::studio {

namespace {

[[nodiscard]] auto trim(std::string_view s) -> std::string {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.remove_suffix(1);
    }
    return std::string(s);
}

} // anonymous namespace

bool GitStatus::is_modified() const {
    return index_status == 'M' || worktree_status == 'M';
}

bool GitStatus::is_added() const {
    return index_status == 'A' || worktree_status == 'A';
}

bool GitStatus::is_deleted() const {
    return index_status == 'D' || worktree_status == 'D';
}

bool GitStatus::is_renamed() const {
    return index_status == 'R' || worktree_status == 'R';
}

bool GitStatus::is_untracked() const {
    return index_status == '?' && worktree_status == '?';
}

auto GitIntegration::exec_git(const std::string& args) const -> std::string {
    std::string cmd = "git -C \"" + repo_path_ + "\" " + args + " 2>&1";
    std::array<char, 4096> buffer{};
    std::string result;
    auto pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return {};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    _pclose(pipe);
    return result;
}

GitIntegration::GitIntegration(std::string repo_path)
    : repo_path_(std::move(repo_path))
{
}

auto GitIntegration::is_git_repo() const -> bool {
    auto out = exec_git("rev-parse --git-dir");
    return !out.empty() && out.find("fatal:") == std::string::npos;
}

auto GitIntegration::current_branch() const -> std::string {
    return trim(exec_git("rev-parse --abbrev-ref HEAD"));
}

auto GitIntegration::status() const -> std::vector<GitStatus> {
    std::vector<GitStatus> result;
    auto out = exec_git("status --porcelain");
    if (out.empty()) return result;

    std::istringstream stream(out);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.size() < 3) continue;
        GitStatus s;
        s.index_status = line[0];
        s.worktree_status = line[1];
        s.file_path = trim(line.substr(2));
        result.push_back(std::move(s));
    }
    return result;
}

auto GitIntegration::log(int count) const -> std::vector<GitCommit> {
    std::vector<GitCommit> result;
    auto fmt = "--format=\"%H|%an|%s|%ci\"";
    auto cmd = "log -" + std::to_string(count) + " " + fmt;
    auto out = exec_git(cmd);
    if (out.empty()) return result;

    std::istringstream stream(out);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) continue;

        GitCommit c;
        auto pos1 = line.find('|');
        if (pos1 == std::string::npos) continue;
        c.hash = line.substr(0, pos1);

        auto pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos) continue;
        c.author = line.substr(pos1 + 1, pos2 - pos1 - 1);

        auto pos3 = line.find('|', pos2 + 1);
        if (pos3 == std::string::npos) {
            c.message = line.substr(pos2 + 1);
        } else {
            c.message = line.substr(pos2 + 1, pos3 - pos2 - 1);
            c.timestamp = line.substr(pos3 + 1);
        }

        result.push_back(std::move(c));
    }
    return result;
}

auto GitIntegration::stage(const std::string& file_path) -> bool {
    auto out = exec_git("add \"" + file_path + "\"");
    return out.empty() || out.find("fatal:") == std::string::npos;
}

auto GitIntegration::unstage(const std::string& file_path) -> bool {
    auto out = exec_git("restore --staged \"" + file_path + "\"");
    return out.empty() || out.find("fatal:") == std::string::npos;
}

auto GitIntegration::commit(const std::string& message) -> bool {
    auto escaped = message;
    // Escape double quotes for shell
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    auto out = exec_git("commit -m \"" + escaped + "\"");
    return out.empty() || out.find("fatal:") == std::string::npos;
}

auto GitIntegration::checkout_branch(const std::string& branch) -> bool {
    auto out = exec_git("checkout \"" + branch + "\"");
    return out.empty() || out.find("fatal:") == std::string::npos;
}

auto GitIntegration::diff(const std::string& file_path) const -> std::string {
    if (file_path.empty()) {
        return exec_git("diff");
    }
    return exec_git("diff \"" + file_path + "\"");
}

auto GitIntegration::diff_staged(const std::string& file_path) const -> std::string {
    if (file_path.empty()) {
        return exec_git("diff --cached");
    }
    return exec_git("diff --cached \"" + file_path + "\"");
}

auto GitIntegration::blame(const std::string& file_path, int line) const -> std::string {
    auto cmd = "blame -L " + std::to_string(line) + "," + std::to_string(line)
             + " \"" + file_path + "\"";
    return trim(exec_git(cmd));
}

} // namespace gspl::studio
