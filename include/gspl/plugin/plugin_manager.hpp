#pragma once

#include "manifest.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace gspl::plugin {

enum class PluginState {
    Discovered,
    Loaded,
    Activated,
    Deactivated,
    Error
};

struct PluginInstance {
    std::string library_path;
    PluginManifest manifest;
    PluginState state{PluginState::Discovered};
    void* library_handle{nullptr};
    std::string error_message;
};

class PluginManager {
public:
    using LogCallback = std::function<void(std::string_view)>;

    PluginManager();
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void set_plugin_directories(std::vector<std::string> dirs);
    void set_log_callback(LogCallback cb);

    bool discover_plugins();
    bool load_plugin(const std::string& id);
    bool activate_plugin(const std::string& id);
    void deactivate_plugin(const std::string& id);
    bool unload_plugin(const std::string& id);

    [[nodiscard]] auto plugins() const -> const std::vector<PluginInstance>&;
    [[nodiscard]] auto find(const std::string& id) -> PluginInstance*;

    void activate_all();
    void deactivate_all();
    void shutdown_all();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::plugin
