#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace gspl::studio {

enum class AuthorityLevel {
    None,
    Read,
    Write,
    Admin
};

struct DirectoryAuthority {
    std::string path;
    AuthorityLevel level{AuthorityLevel::None};
    bool recursive{true};
};

class AuthorityModel {
public:
    AuthorityModel();
    explicit AuthorityModel(std::string workspace_root);
    ~AuthorityModel();

    AuthorityModel(const AuthorityModel&) = delete;
    AuthorityModel& operator=(const AuthorityModel&) = delete;

    bool grant_access(const std::string& path, AuthorityLevel level, bool recursive = true);
    bool revoke_access(const std::string& path);
    [[nodiscard]] AuthorityLevel check_access(const std::string& path) const;

    [[nodiscard]] auto authorities() const -> const std::vector<DirectoryAuthority>&;
    void set_workspace_root(const std::string& root);
    [[nodiscard]] auto workspace_root() const -> const std::string&;

    void reset_to_defaults();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
