#pragma once

#include <string>
#include <vector>
#include <memory>

namespace gspl::studio {

struct TargetProfile {
    std::string name;
    std::vector<std::string> compile_flags;
    std::vector<std::string> link_flags;
    std::string output_format;
};

struct TargetAdapterInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> supported_platforms;
    std::vector<TargetProfile> profiles;
    bool requires_sdk{false};
    std::string sdk_name;
    std::string sdk_min_version;
};

class TargetAdapterManager {
public:
    TargetAdapterManager();
    ~TargetAdapterManager();

    bool register_adapter(TargetAdapterInfo info);
    bool unregister_adapter(const std::string& id);

    [[nodiscard]] auto adapters() const -> const std::vector<TargetAdapterInfo>&;
    [[nodiscard]] auto find(const std::string& id) -> TargetAdapterInfo*;

    bool detect_sdk(const std::string& adapter_id);
    bool set_active_profile(const std::string& adapter_id, const std::string& profile_name);
    [[nodiscard]] auto active_profile(const std::string& adapter_id) const -> std::string;

    void load_builtin_adapters();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
