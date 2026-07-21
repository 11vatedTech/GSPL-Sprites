#include "gspl/cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    gspl::Cli cli;
    auto parsed = cli.parse(argc, argv);
    if (!parsed.success) {
        if (!parsed.error.empty()) std::cerr << "gsplc: " << parsed.error << "\n";
        return 1;
    }
    if (parsed.options.input_files.empty()) return 0;
    return cli.run(parsed.options);
}
