#pragma once
#include "gspl/passes.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gspl {

struct CliOptions {
    std::vector<std::filesystem::path> input_files;
    std::vector<std::filesystem::path> source_roots;
    std::filesystem::path output_dir;
    bool emit_json{false};
    bool emit_ir{false};
    bool validate_only{false};
    bool verbose{false};
    bool package_output{false};
    bool verify{false};
    std::filesystem::path package_dir;
    bool deterministic_seed{false};
    std::uint64_t deterministic_entropy{42};
    std::optional<std::uint32_t> max_threads;
    std::vector<PassKind> stop_after;
    std::string model_id;
    bool preserve_artifact_cache{true};
    bool no_network{true};
    bool migrate{false};
    std::filesystem::path migrate_output_dir;
    bool graph{false};
    bool migrate_dry_run{false};
    bool migrate_overwrite{false};
};

struct SourceFile {
    std::filesystem::path path;
    bool is_stdin{false};
};

class Cli {
public:
    explicit Cli() = default;
    struct ParseResult {
        bool success{false};
        CliOptions options;
        std::string error;
    };
    ParseResult parse(int argc, char* argv[]);
    int run(CliOptions const& options);
    static void print_help();
    static void print_version();
    static void print_diagnostic(Diagnostic const& diag, SourceManager const& sources);
private:
    static constexpr auto version_ = "gsplc 1.0.0";
    DiagnosticResult compile_file(std::filesystem::path const& path, CliOptions const& opts);
    DiagnosticResult compile_source(SourceBuffer source, CliOptions const& opts);
    bool emit_output(CompilationContext const& ctx, CliOptions const& opts);
};

} // namespace gspl
