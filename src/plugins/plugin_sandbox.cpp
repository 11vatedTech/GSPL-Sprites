#include "gspl/plugin/plugin_sandbox.hpp"
#include "gspl/plugin/plugin_api.h"
#include "gspl/studio/ipc_channel.hpp"
#include <atomic>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace gspl::plugin {

namespace {

void set_stdin_binary() {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
}

std::string json_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (auto ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch;
        }
    }
    return out;
}

} // anonymous namespace

struct PluginSandbox::Impl {
    SandboxConfig config;
    PluginManager manager;
    std::unique_ptr<studio::IpcChannel> channel;
    std::atomic<bool> running{false};
    std::thread reader_thread;
    EnvelopeHandler custom_handler;

    Impl(SandboxConfig cfg) : config(std::move(cfg)) {}

    void setup_channel() {
        // Use stdin/stdout for IPC
        int read_fd = 0;  // stdin
        int write_fd = 1; // stdout
        set_stdin_binary();
#ifdef _WIN32
        read_fd = _fileno(stdin);
        write_fd = _fileno(stdout);
#else
        read_fd = STDIN_FILENO;
        write_fd = STDOUT_FILENO;
#endif
        channel = std::make_unique<studio::IpcChannel>(read_fd, write_fd);
    }

    void reader_loop(PluginSandbox* sandbox) {
        while (running) {
            auto env = channel->receive(std::chrono::milliseconds(200));
            if (!env.method.empty() || env.message_id != 0) {
                if (custom_handler) {
                    custom_handler(env);
                } else {
                    sandbox->handle_envelope(env);
                }
            }
        }
    }
};

PluginSandbox::PluginSandbox(SandboxConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

PluginSandbox::~PluginSandbox() {
    shutdown();
}

bool PluginSandbox::initialize() {
    impl_->setup_channel();
    impl_->manager.set_plugin_directories(impl_->config.plugin_directories);
    impl_->manager.discover_plugins();
    return true;
}

void PluginSandbox::run() {
    impl_->running = true;
    impl_->reader_thread = std::thread(&Impl::reader_loop, impl_.get(), this);
    impl_->reader_thread.join();
}

void PluginSandbox::shutdown() {
    impl_->running = false;
    if (impl_->reader_thread.joinable()) {
        impl_->reader_thread.join();
    }
    impl_->manager.shutdown_all();
    impl_->channel.reset();
}

auto PluginSandbox::manager() -> PluginManager& {
    return impl_->manager;
}

void PluginSandbox::handle_envelope(const studio::IpcEnvelope& env) {
    if (env.method == "ping") {
        studio::IpcEnvelope pong;
        pong.message_id = env.message_id;
        pong.method = "pong";
        pong.payload = "{}";
        impl_->channel->send(pong);
        return;
    }

    if (env.method == "shutdown") {
        send_response(env.message_id, R"({"status":"ok"})");
        impl_->running = false;
        return;
    }

    if (env.method == "get_info") {
        std::ostringstream oss;
        oss << "{\"plugins\":[";
        bool first = true;
        for (auto const& p : impl_->manager.plugins()) {
            if (!first) oss << ",";
            first = false;
            oss << "{\"id\":\"" << json_escape(p.manifest.id)
                << "\",\"version\":\"" << json_escape(p.manifest.version)
                << "\",\"name\":\"" << json_escape(p.manifest.name)
                << "\",\"state\":\"" << static_cast<int>(p.state)
                << "\"}";
        }
        oss << "]}";
        send_response(env.message_id, oss.str());
        return;
    }

    if (env.method == "load_plugin") {
        std::string id = env.payload;
        bool ok = impl_->manager.load_plugin(id);
        if (ok) {
            ok = impl_->manager.activate_plugin(id);
        }
        if (ok) {
            send_response(env.message_id, R"({"status":"ok"})");
        } else {
            send_error(env.message_id, "Failed to load/activate plugin: " + id);
        }
        return;
    }

    if (env.method == "unload_plugin") {
        impl_->manager.unload_plugin(env.payload);
        send_response(env.message_id, R"({"status":"ok"})");
        return;
    }

    send_error(env.message_id, "Unknown method: " + env.method);
}

void PluginSandbox::send_response(std::uint64_t message_id, std::string_view payload) {
    studio::IpcEnvelope response;
    response.message_id = message_id;
    response.method = "response";
    response.payload = std::string(payload);
    impl_->channel->send(response);
}

void PluginSandbox::send_error(std::uint64_t message_id, std::string_view message) {
    studio::IpcEnvelope response;
    response.message_id = message_id;
    response.method = "error";
    response.payload = std::string(message);
    impl_->channel->send(response);
}

} // namespace gspl::plugin
