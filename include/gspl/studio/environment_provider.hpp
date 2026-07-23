#pragma once

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gspl::studio {

struct EnvironmentEntry {
    std::string value;
    bool is_set{false};
    bool is_valid_path{false};
};

class EnvironmentProvider {
public:
    explicit EnvironmentProvider(std::vector<std::string> allowlist = default_allowlist());

    EnvironmentEntry get(std::string_view name) const;
    EnvironmentEntry get_path(std::string_view name) const;

    void set_override(std::string_view name, std::string_view value);
    void clear_overrides();
    void set_allowlist(std::vector<std::string> names);

    static std::vector<std::string> default_allowlist();

private:
    EnvironmentEntry read_env(std::string_view name) const;
    bool is_allowed(std::string_view name) const;

    std::vector<std::string> allowlist_;
    std::unordered_map<std::string, std::string> overrides_;
};

} // namespace gspl::studio
