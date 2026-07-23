#include "gspl/studio/target_adapter.hpp"
#include "gspl/studio/environment_provider.hpp"
#include <algorithm>
#include <cstdlib>

namespace gspl::studio {

namespace {

[[nodiscard]] auto make_desktop_runtime_v1() -> TargetAdapterInfo {
    TargetAdapterInfo info;
    info.id = "desktop-runtime-v1";
    info.name = "Desktop Runtime v1";
    info.version = "1.0.0";
    info.description = "Standard GSPL runtime for desktop platforms";
    info.supported_platforms = {"windows", "linux"};
    info.requires_sdk = false;

    TargetProfile debug;
    debug.name = "debug";
    debug.compile_flags = {"-O0", "-g", "-DDEBUG"};
    debug.link_flags = {};
    debug.output_format = "executable";

    TargetProfile release;
    release.name = "release";
    release.compile_flags = {"-O2", "-DNDEBUG"};
    release.link_flags = {"-s"};
    release.output_format = "executable";

    TargetProfile minsize;
    minsize.name = "minsize";
    minsize.compile_flags = {"-Oz", "-DNDEBUG"};
    minsize.link_flags = {"-s"};
    minsize.output_format = "executable";

    info.profiles = {std::move(debug), std::move(release), std::move(minsize)};
    return info;
}

[[nodiscard]] auto default_env() -> EnvironmentProvider& {
    static EnvironmentProvider env(EnvironmentProvider::default_allowlist());
    return env;
}

[[nodiscard]] auto check_env_path(const std::string& var) -> bool {
    auto entry = default_env().get_path(var);
    return entry.is_set && entry.is_valid_path;
}

} // anonymous namespace

struct TargetAdapterManager::Impl {
    std::vector<TargetAdapterInfo> adapters;
};

TargetAdapterManager::TargetAdapterManager()
    : impl_(std::make_unique<Impl>())
{
}

TargetAdapterManager::~TargetAdapterManager() = default;

bool TargetAdapterManager::register_adapter(TargetAdapterInfo info) {
    auto existing = std::find_if(impl_->adapters.begin(), impl_->adapters.end(),
        [&](const auto& a) { return a.id == info.id; });
    if (existing != impl_->adapters.end()) {
        return false;
    }
    impl_->adapters.push_back(std::move(info));
    return true;
}

bool TargetAdapterManager::unregister_adapter(const std::string& id) {
    auto it = std::find_if(impl_->adapters.begin(), impl_->adapters.end(),
        [&](const auto& a) { return a.id == id; });
    if (it == impl_->adapters.end()) {
        return false;
    }
    impl_->adapters.erase(it);
    return true;
}

auto TargetAdapterManager::adapters() const -> const std::vector<TargetAdapterInfo>& {
    return impl_->adapters;
}

auto TargetAdapterManager::find(const std::string& id) -> TargetAdapterInfo* {
    for (auto& a : impl_->adapters) {
        if (a.id == id) return &a;
    }
    return nullptr;
}

bool TargetAdapterManager::detect_sdk(const std::string& adapter_id) {
    auto* adapter = find(adapter_id);
    if (!adapter) return false;
    if (!adapter->requires_sdk) return true;

    if (!adapter->sdk_name.empty()) {
        if (check_env_path(adapter->sdk_name + "_HOME")) return true;
        if (check_env_path(adapter->sdk_name + "_ROOT")) return true;
    }
    if (!adapter->sdk_min_version.empty()) {
        auto ver_var = adapter->sdk_name + "_VERSION";
        auto entry = default_env().get(ver_var);
        if (entry.is_set && entry.value >= adapter->sdk_min_version) {
            return true;
        }
    }
    return false;
}

bool TargetAdapterManager::set_active_profile(const std::string& adapter_id, const std::string& profile_name) {
    auto* adapter = find(adapter_id);
    if (!adapter) return false;

    auto it = std::find_if(adapter->profiles.begin(), adapter->profiles.end(),
        [&](const auto& p) { return p.name == profile_name; });
    if (it == adapter->profiles.end()) return false;

    return true;
}

auto TargetAdapterManager::active_profile(const std::string& adapter_id) const -> std::string {
    for (const auto& a : impl_->adapters) {
        if (a.id == adapter_id && !a.profiles.empty()) {
            return a.profiles.front().name;
        }
    }
    return {};
}

void TargetAdapterManager::load_builtin_adapters() {
    register_adapter(make_desktop_runtime_v1());
}

} // namespace gspl::studio
