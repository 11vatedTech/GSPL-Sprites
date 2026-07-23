#pragma once

#include "gspl/plugin/plugin_api.h"
#include <string>
#include <vector>
#include <optional>

namespace gspl::plugin {

struct PluginDependency {
    std::string id;
    std::string version_constraint; // semver constraint
};

struct PluginManifest {
    std::string id;
    std::string version;
    std::string name;
    std::string description;
    std::string author;
    std::string license;
    uint32_t api_version{GSPL_PLUGIN_API_VERSION};
    std::vector<PluginDependency> dependencies;
    std::vector<std::string> capabilities; // hook points this plugin uses
    
    static auto load(const std::string& path) -> std::optional<PluginManifest>;
    [[nodiscard]] bool validate() const;
    [[nodiscard]] std::string to_json() const;
};

} // namespace gspl::plugin
