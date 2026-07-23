#include "gspl/plugin/plugin_sandbox.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    gspl::plugin::SandboxConfig config;
    config.worker_id = "plugin-sandbox";

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--plugin-dir" && i + 1 < argc) {
            config.plugin_directories.push_back(argv[++i]);
        } else if (arg == "--worker-id" && i + 1 < argc) {
            config.worker_id = argv[++i];
        }
    }

    if (config.plugin_directories.empty()) {
        config.plugin_directories.push_back(".");
    }

    gspl::plugin::PluginSandbox sandbox(std::move(config));

    if (!sandbox.initialize()) {
        std::cerr << "Plugin sandbox initialization failed" << std::endl;
        return 1;
    }

    sandbox.run();
    return 0;
}
