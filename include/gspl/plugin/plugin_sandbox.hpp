#pragma once

#include "gspl/plugin/plugin_manager.hpp"
#include "gspl/studio/ipc_envelope.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::plugin {

struct SandboxConfig {
    std::vector<std::string> plugin_directories;
    std::string worker_id;
    int restart_limit{3};
};

class PluginSandbox {
public:
    using EnvelopeHandler = std::function<void(const studio::IpcEnvelope&)>;

    explicit PluginSandbox(SandboxConfig config);
    ~PluginSandbox();

    PluginSandbox(const PluginSandbox&) = delete;
    PluginSandbox& operator=(const PluginSandbox&) = delete;

    bool initialize();
    void run();
    void shutdown();

    auto manager() -> PluginManager&;

private:
    void handle_envelope(const studio::IpcEnvelope& env);
    void send_response(std::uint64_t message_id, std::string_view payload);
    void send_error(std::uint64_t message_id, std::string_view message);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::plugin
