#define _CRT_SECURE_NO_WARNINGS
#include "gspl/studio/environment_provider.hpp"

namespace gspl::studio {

EnvironmentProvider::EnvironmentProvider(std::vector<std::string> allowlist)
    : allowlist_(std::move(allowlist)) {}

std::vector<std::string> EnvironmentProvider::default_allowlist() {
    return {
        "USERPROFILE", "HOME", "LOCALAPPDATA",
        "APPDATA", "ProgramFiles", "ProgramFiles(x86)",
        "TEMP", "TMP", "GSPL_HOME",
        "GSPL_CACHE_DIR", "GSPL_PLUGIN_PATH"
    };
}

bool EnvironmentProvider::is_allowed(std::string_view name) const {
    return std::ranges::any_of(allowlist_, [&](const std::string& allowed) {
        return allowed == name;
    });
}

EnvironmentEntry EnvironmentProvider::read_env(std::string_view name) const {
    EnvironmentEntry entry;
    if (!is_allowed(name)) return entry;
    auto it = overrides_.find(std::string(name));
    if (it != overrides_.end()) {
        entry.value = it->second;
        entry.is_set = true;
        return entry;
    }
    auto val = std::getenv(std::string(name).c_str());
    if (val) {
        entry.value = val;
        entry.is_set = true;
    }
    return entry;
}

EnvironmentEntry EnvironmentProvider::get(std::string_view name) const {
    return read_env(name);
}

EnvironmentEntry EnvironmentProvider::get_path(std::string_view name) const {
    auto entry = read_env(name);
    if (entry.is_set && !entry.value.empty()) {
        std::error_code ec;
        entry.is_valid_path = std::filesystem::exists(entry.value, ec);
    }
    return entry;
}

void EnvironmentProvider::set_override(std::string_view name, std::string_view value) {
    overrides_[std::string(name)] = std::string(value);
}

void EnvironmentProvider::clear_overrides() {
    overrides_.clear();
}

void EnvironmentProvider::set_allowlist(std::vector<std::string> names) {
    allowlist_ = std::move(names);
}

} // namespace gspl::studio
