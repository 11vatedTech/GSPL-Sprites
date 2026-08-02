#include "gspl/cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    gspl::Cli cli;
    auto parsed = cli.parse(argc, argv);
    if (!parsed.success) {
        if (!parsed.error.empty()) std::cerr << "gsplc: " << parsed.error << "\n";
        return 1;
    }
    // Package verification runs standalone: no source input required.
    if (parsed.options.verify_package) {
        if (parsed.options.verify_package_path.empty()) {
            std::cerr << "gsplc: --verify-package requires a package path\n";
            return 1;
        }
        if (!parsed.options.input_files.empty()) return 1;
        return cli.run(parsed.options);
    }
    if (parsed.options.input_files.empty() && !parsed.options.graph && !parsed.options.migrate) return 0;
    return cli.run(parsed.options);
}
