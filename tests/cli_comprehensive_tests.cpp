#include "gspl/cli.hpp"
#include "gspl/legacy.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. CLI parse: help flag ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--help")};
            gspl::Cli cli;
            auto result = cli.parse(2, args);
            check(result.success, "--help should succeed");
            check(result.options.input_files.empty(), "--help should produce no input files");
        }

        // ---- 2. CLI parse: version flag ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--version")};
            gspl::Cli cli;
            auto result = cli.parse(2, args);
            check(result.success, "--version should succeed");
        }

        // ---- 3. CLI parse: input file ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("test.gspl")};
            gspl::Cli cli;
            auto result = cli.parse(2, args);
            check(result.success, "input file should parse");
            check(result.options.input_files.size() == 1, "should have 1 input file");
        }

        // ---- 4. CLI parse: multiple flags ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--json"),
                            const_cast<char*>("--emit-ir"), const_cast<char*>("--verbose"),
                            const_cast<char*>("--deterministic"), const_cast<char*>("test.gspl")};
            gspl::Cli cli;
            auto result = cli.parse(6, args);
            check(result.success, "multiple flags should parse");
            check(result.options.emit_json, "--json should be set");
            check(result.options.emit_ir, "--emit-ir should be set");
            check(result.options.verbose, "--verbose should be set");
            check(result.options.deterministic_seed, "--deterministic should be set");
        }

        // ---- 5. CLI parse: output dir ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("-o"),
                            const_cast<char*>("outdir"), const_cast<char*>("test.gspl")};
            gspl::Cli cli;
            auto result = cli.parse(4, args);
            check(result.success, "output dir should parse");
            check(result.options.output_dir == "outdir", "output dir should be set");
        }

        // ---- 6. CLI parse: stop-after ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--stop-after=lex"),
                            const_cast<char*>("test.gspl")};
            gspl::Cli cli;
            auto result = cli.parse(3, args);
            check(result.success, "stop-after should parse");
            check(!result.options.stop_after.empty(), "stop_after should not be empty");
        }

        // ---- 7. CLI parse: unknown option ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--nonexistent"),
                            const_cast<char*>("test.gspl")};
            gspl::Cli cli;
            auto result = cli.parse(3, args);
            check(!result.success, "unknown option should fail");
            check(!result.error.empty(), "error message should be non-empty");
        }

        // ---- 8. CLI parse: migrate flags ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--migrate"),
                            const_cast<char*>("--migrate-dry-run"),
                            const_cast<char*>("--migrate-overwrite"),
                            const_cast<char*>("--migrate-output"), const_cast<char*>("out"),
                            const_cast<char*>("test.sprite")};
            gspl::Cli cli;
            auto result = cli.parse(7, args);
            check(result.success, "migrate flags should parse");
            check(result.options.migrate, "--migrate should be set");
            check(result.options.migrate_dry_run, "--migrate-dry-run should be set");
            check(result.options.migrate_overwrite, "--migrate-overwrite should be set");
            check(result.options.migrate_output_dir == "out", "migrate output dir should be set");
        }

        // ---- 9. CLI parse: graph flag ----
        {
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--graph")};
            gspl::Cli cli;
            auto result = cli.parse(2, args);
            check(result.success, "--graph should parse");
            check(result.options.graph, "--graph should be set");
        }

        // ---- 10. CLI run: migrate dry-run ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_cli_test_migrate";
            std::filesystem::create_directories(tmp);
            auto input = tmp / "test.sprite";
            {
                std::ofstream f(input);
                f << "sprite Test\n";
                f << "stable_id: \"test.migrate\"\n";
                f << "classification: \"test.migration\"\n";
            }
            char* args[] = {const_cast<char*>("gsplc"), const_cast<char*>("--migrate"),
                            const_cast<char*>("--migrate-dry-run"),
                            const_cast<char*>(input.string().c_str())};
            gspl::Cli cli;
            auto result = cli.parse(3, args);
            check(result.success, "migrate dry-run should parse");
            auto exit_code = cli.run(result.options);
            check(exit_code == 0, "migrate dry-run should exit 0");
            std::filesystem::remove_all(tmp);
        }

        // ---- 11. print_version returns string ----
        {
            gspl::Cli::print_version();
            gspl::Cli::print_help();
        }

        std::cout << "ALL CLI COMPREHENSIVE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
