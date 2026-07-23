#include "gspl/plugin/plugin_manager.hpp"
#include "gspl/plugin/plugin_api.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace gspl::plugin {
namespace fs = std::filesystem;

struct PluginManager::Impl {
    std::vector<std::string> search_dirs;
    std::vector<PluginInstance> instances;
    LogCallback log_cb;

    void log(std::string_view msg) {
        if (log_cb) log_cb(msg);
    }

    static PluginInstance scan_library(const fs::path& lib_path) {
        PluginInstance inst;
        inst.library_path = lib_path.string();
        inst.state = PluginState::Discovered;

#ifdef _WIN32
        HMODULE handle = LoadLibraryW(lib_path.c_str());
        if (!handle) {
            inst.state = PluginState::Error;
            inst.error_message = "LoadLibrary failed";
            return inst;
        }
        auto get_info = reinterpret_cast<GsplPluginInitFunc>(
            GetProcAddress(handle, "gspl_plugin_init"));
        if (!get_info) {
            FreeLibrary(handle);
            inst.state = PluginState::Error;
            inst.error_message = "Symbol gspl_plugin_init not found";
            return inst;
        }
        GsplPluginInfo info{};
        GsplPluginCallbacks callbacks{};
        if (get_info(&info, &callbacks, nullptr) != 0) {
            FreeLibrary(handle);
            inst.state = PluginState::Error;
            inst.error_message = "gspl_plugin_init returned failure";
            return inst;
        }
        if (callbacks.initialize) inst.manifest.capabilities.push_back("initialize");
        if (callbacks.shutdown) inst.manifest.capabilities.push_back("shutdown");
        if (callbacks.on_document_open) inst.manifest.capabilities.push_back("on_document_open");
        if (callbacks.on_document_close) inst.manifest.capabilities.push_back("on_document_close");
        if (callbacks.on_diagnostic) inst.manifest.capabilities.push_back("on_diagnostic");
        if (callbacks.on_build_start) inst.manifest.capabilities.push_back("on_build_start");
        if (callbacks.on_build_end) inst.manifest.capabilities.push_back("on_build_end");
        if (callbacks.get_tool_window_title) inst.manifest.capabilities.push_back("get_tool_window_title");
        if (callbacks.render_tool_window) inst.manifest.capabilities.push_back("render_tool_window");
        FreeLibrary(handle);
#else
        void* handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            inst.state = PluginState::Error;
            inst.error_message = dlerror();
            return inst;
        }
        auto get_info = reinterpret_cast<GsplPluginInitFunc>(
            dlsym(handle, "gspl_plugin_init"));
        if (!get_info) {
            dlclose(handle);
            inst.state = PluginState::Error;
            inst.error_message = "Symbol gspl_plugin_init not found";
            return inst;
        }
        GsplPluginInfo info{};
        GsplPluginCallbacks callbacks{};
        if (get_info(&info, &callbacks, nullptr) != 0) {
            dlclose(handle);
            inst.state = PluginState::Error;
            inst.error_message = "gspl_plugin_init returned failure";
            return inst;
        }
        if (callbacks.initialize) inst.manifest.capabilities.push_back("initialize");
        if (callbacks.shutdown) inst.manifest.capabilities.push_back("shutdown");
        if (callbacks.on_document_open) inst.manifest.capabilities.push_back("on_document_open");
        if (callbacks.on_document_close) inst.manifest.capabilities.push_back("on_document_close");
        if (callbacks.on_diagnostic) inst.manifest.capabilities.push_back("on_diagnostic");
        if (callbacks.on_build_start) inst.manifest.capabilities.push_back("on_build_start");
        if (callbacks.on_build_end) inst.manifest.capabilities.push_back("on_build_end");
        if (callbacks.get_tool_window_title) inst.manifest.capabilities.push_back("get_tool_window_title");
        if (callbacks.render_tool_window) inst.manifest.capabilities.push_back("render_tool_window");
        dlclose(handle);
#endif
        inst.manifest.id = info.plugin_id ? info.plugin_id : "";
        inst.manifest.version = info.plugin_version ? info.plugin_version : "";
        inst.manifest.name = info.plugin_name ? info.plugin_name : "";
        inst.manifest.description = info.plugin_description ? info.plugin_description : "";
        inst.manifest.author = info.plugin_author ? info.plugin_author : "";
        inst.manifest.api_version = info.api_version;

        return inst;
    }

    PluginInstance* find_by_id(const std::string& id) {
        for (auto& inst : instances) {
            if (inst.manifest.id == id) return &inst;
        }
        return nullptr;
    }
};

PluginManager::PluginManager() : impl_(std::make_unique<Impl>()) {}
PluginManager::~PluginManager() = default;

void PluginManager::set_plugin_directories(std::vector<std::string> dirs) {
    impl_->search_dirs = std::move(dirs);
}

void PluginManager::set_log_callback(LogCallback cb) {
    impl_->log_cb = std::move(cb);
}

bool PluginManager::discover_plugins() {
    bool found_any = false;
    for (auto const& dir : impl_->search_dirs) {
        fs::path dir_path(dir);
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) continue;
        for (auto const& entry : fs::directory_iterator(dir_path)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
#ifdef _WIN32
            if (ext != ".dll") continue;
#else
            if (ext != ".so") continue;
#endif
            auto inst = Impl::scan_library(entry.path());
            if (inst.state != PluginState::Error) {
                auto existing = impl_->find_by_id(inst.manifest.id);
                if (!existing) {
                    impl_->instances.push_back(std::move(inst));
                    found_any = true;
                }
            }
        }
    }
    return found_any;
}

bool PluginManager::load_plugin(const std::string& id) {
    auto* inst = impl_->find_by_id(id);
    if (!inst) {
        impl_->log("Plugin not found: " + id);
        return false;
    }
    if (inst->state == PluginState::Loaded || inst->state == PluginState::Activated) {
        return true;
    }
    if (inst->state == PluginState::Error) return false;

    const auto& lib_path = inst->library_path;
#ifdef _WIN32
    HMODULE handle = LoadLibraryW(fs::path(lib_path).c_str());
    if (!handle) {
        inst->state = PluginState::Error;
        inst->error_message = "LoadLibrary failed for " + lib_path;
        return false;
    }
#else
    void* handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        inst->state = PluginState::Error;
        inst->error_message = dlerror();
        return false;
    }
#endif
    inst->library_handle = handle;
    inst->state = PluginState::Loaded;
    return true;
}

bool PluginManager::activate_plugin(const std::string& id) {
    auto* inst = impl_->find_by_id(id);
    if (!inst) return false;
    if (inst->state == PluginState::Activated) return true;
    if (inst->state != PluginState::Loaded) {
        if (!load_plugin(id)) return false;
        inst = impl_->find_by_id(id);
        if (!inst) return false;
    }

    // Resolve dependencies first
    for (auto const& dep : inst->manifest.dependencies) {
        auto* dep_inst = impl_->find_by_id(dep.id);
        if (!dep_inst) {
            inst->error_message = "Missing dependency: " + dep.id;
            inst->state = PluginState::Error;
            return false;
        }
        if (dep_inst->state != PluginState::Activated) {
            if (!activate_plugin(dep.id)) {
                inst->error_message = "Failed to activate dependency: " + dep.id;
                inst->state = PluginState::Error;
                return false;
            }
        }
    }

#ifdef _WIN32
    auto init_fn = reinterpret_cast<GsplPluginInitFunc>(
        GetProcAddress(static_cast<HMODULE>(inst->library_handle), "gspl_plugin_init"));
#else
    auto init_fn = reinterpret_cast<GsplPluginInitFunc>(
        dlsym(inst->library_handle, "gspl_plugin_init"));
#endif
    if (!init_fn) {
        inst->error_message = "Symbol gspl_plugin_init lost after loading";
        inst->state = PluginState::Error;
        return false;
    }

    // Get shutdown func too
#ifdef _WIN32
    auto shutdown_fn = reinterpret_cast<GsplPluginShutdownFunc>(
        GetProcAddress(static_cast<HMODULE>(inst->library_handle), "gspl_plugin_shutdown"));
#else
    auto shutdown_fn = reinterpret_cast<GsplPluginShutdownFunc>(
        dlsym(inst->library_handle, "gspl_plugin_shutdown"));
#endif

    GsplPluginInfo info{};
    info.api_version = inst->manifest.api_version;
    info.plugin_id = inst->manifest.id.c_str();
    info.plugin_version = inst->manifest.version.c_str();
    info.plugin_name = inst->manifest.name.c_str();
    info.plugin_description = inst->manifest.description.c_str();
    info.plugin_author = inst->manifest.author.c_str();

    GsplPluginCallbacks cbs{};
    cbs.initialize = nullptr;
    cbs.shutdown = reinterpret_cast<void(*)(void*)>(shutdown_fn);

    int result = init_fn(&info, &cbs, nullptr);
    if (result != 0) {
        inst->error_message = "Plugin init returned " + std::to_string(result);
        inst->state = PluginState::Error;
        return false;
    }

    if (cbs.initialize) {
        result = cbs.initialize(nullptr);
        if (result != 0) {
            inst->error_message = "Plugin initialize returned " + std::to_string(result);
            inst->state = PluginState::Error;
            return false;
        }
    }

    inst->state = PluginState::Activated;
    return true;
}

void PluginManager::deactivate_plugin(const std::string& id) {
    auto* inst = impl_->find_by_id(id);
    if (!inst || inst->state != PluginState::Activated) return;

#ifdef _WIN32
    auto shutdown_fn = reinterpret_cast<GsplPluginShutdownFunc>(
        GetProcAddress(static_cast<HMODULE>(inst->library_handle), "gspl_plugin_shutdown"));
#else
    auto shutdown_fn = reinterpret_cast<GsplPluginShutdownFunc>(
        dlsym(inst->library_handle, "gspl_plugin_shutdown"));
#endif
    if (shutdown_fn) shutdown_fn(nullptr);

    inst->state = PluginState::Deactivated;
}

bool PluginManager::unload_plugin(const std::string& id) {
    auto* inst = impl_->find_by_id(id);
    if (!inst) return false;

    if (inst->state == PluginState::Activated) {
        deactivate_plugin(id);
    }
    if (inst->library_handle) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(inst->library_handle));
#else
        dlclose(inst->library_handle);
#endif
        inst->library_handle = nullptr;
    }
    inst->state = PluginState::Discovered;

    auto it = std::remove_if(impl_->instances.begin(), impl_->instances.end(),
        [&](const PluginInstance& p) { return p.manifest.id == id; });
    impl_->instances.erase(it, impl_->instances.end());
    return true;
}

auto PluginManager::plugins() const -> const std::vector<PluginInstance>& {
    return impl_->instances;
}

auto PluginManager::find(const std::string& id) -> PluginInstance* {
    return impl_->find_by_id(id);
}

void PluginManager::activate_all() {
    for (auto& inst : impl_->instances) {
        if (inst.state == PluginState::Loaded) {
            activate_plugin(inst.manifest.id);
        }
    }
}

void PluginManager::deactivate_all() {
    // Deactivate in reverse order to respect dependency ordering
    for (auto it = impl_->instances.rbegin(); it != impl_->instances.rend(); ++it) {
        if (it->state == PluginState::Activated) {
            deactivate_plugin(it->manifest.id);
        }
    }
}

void PluginManager::shutdown_all() {
    deactivate_all();
    for (auto& inst : impl_->instances) {
        if (inst.library_handle) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(inst.library_handle));
#else
            dlclose(inst.library_handle);
#endif
            inst.library_handle = nullptr;
        }
        inst.state = PluginState::Discovered;
    }
    impl_->instances.clear();
}

} // namespace gspl::plugin
