#include "gspl/cli.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ranges>

namespace gspl {

Cli::ParseResult Cli::parse(int argc, char* argv[]) {
    ParseResult result;
    CliOptions opts;

    for (std::size_t i = 1; i < static_cast<std::size_t>(argc); ++i) {
        std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") { print_help(); result.success = true; return result; }
        if (arg == "--version" || arg == "-v") { print_version(); result.success = true; return result; }
        if (arg == "--output" || arg == "-o") {
            if (++i < static_cast<std::size_t>(argc)) opts.output_dir = argv[i];
            continue;
        }
        if (arg == "--source-root" || arg == "-I") {
            if (++i < static_cast<std::size_t>(argc)) opts.source_roots.push_back(argv[i]);
            continue;
        }
        if (arg == "--json") { opts.emit_json = true; continue; }
        if (arg == "--emit-ir") { opts.emit_ir = true; continue; }
        if (arg == "--validate-only") { opts.validate_only = true; continue; }
        if (arg == "--verbose") { opts.verbose = true; continue; }
        if (arg == "--deterministic") { opts.deterministic_seed = true; continue; }
        if (arg == "--model-id") {
            if (++i < static_cast<std::size_t>(argc)) opts.model_id = argv[i];
            continue;
        }
        if (arg.starts_with("--stop-after=")) {
            auto val = arg.substr(13);
            if (val == "lex") opts.stop_after = {PassKind::lex};
            else if (val == "parse") opts.stop_after = {PassKind::parse};
            else if (val == "type-check") opts.stop_after = {PassKind::type_check};
            else if (val == "ir") opts.stop_after = {PassKind::ir_gen};
            continue;
        }
        if (arg.starts_with('-')) {
            result.error = "Unknown option: " + std::string(arg);
            return result;
        }
        opts.input_files.push_back(std::filesystem::path(arg));
    }
    result.success = true;
    result.options = std::move(opts);
    return result;
}

int Cli::run(CliOptions const& options) {
    if (options.input_files.empty()) {
        std::cerr << "gsplc: no input files\n";
        return 1;
    }
    bool all_ok = true;
    for (auto const& file : options.input_files) {
        auto result = compile_file(file, options);
        if (std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](auto const& d) {
                return d.severity >= DiagnosticSeverity::error;
            })) all_ok = false;
        for (auto const& d : result.diagnostics) {
            print_diagnostic(d, {});
        }
    }
    return all_ok ? 0 : 1;
}

DiagnosticResult Cli::compile_file(std::filesystem::path const& path, CliOptions const& opts) {
    try {
        auto buffer = SourceBuffer::from_file(path);
        return compile_source(std::move(buffer), opts);
    } catch (std::exception const& e) {
        DiagnosticResult dr;
        dr.add_error(DiagnosticCode::GSPL_MODULE_UNRESOLVED,
                     "Cannot open file: " + std::string(e.what()), {});
        return dr;
    }
}

DiagnosticResult Cli::compile_source(SourceBuffer source, CliOptions const& opts) {
    CompilationContext ctx;
    ctx.expr_config.deterministic_seed = opts.deterministic_seed;
    ctx.expr_config.deterministic_entropy = opts.deterministic_entropy;

    ctx.sources.register_buffer(std::move(source));
    for (auto const& root : opts.source_roots) ctx.sources.add_source_root(root);

    PassManager pm;
    pm.register_pass(std::make_unique<LexPhase>());
    pm.register_pass(std::make_unique<ParsePhase>());
    pm.register_pass(std::make_unique<ModuleResolvePhase>());
    pm.register_pass(std::make_unique<NameResolvePhase>());
    pm.register_pass(std::make_unique<TypeCheckPhase>());
    pm.register_pass(std::make_unique<GeneCompositionPhase>());
    pm.register_pass(std::make_unique<IrGenPhase>());
    pm.register_pass(std::make_unique<IrValidatePhase>());
    pm.register_pass(std::make_unique<IrOptimizePhase>());
    pm.register_pass(std::make_unique<CanonicalizePhase>());
    pm.register_pass(std::make_unique<CanonicalValidatePhase>());
    pm.register_pass(std::make_unique<SpriteIrLowerPhase>());
    pm.register_pass(std::make_unique<SeedLowerPhase>());

    std::vector<PassKind> targets;
    if (opts.stop_after.empty()) {
        targets = {PassKind::lex, PassKind::parse, PassKind::module_resolve,
                   PassKind::name_resolve, PassKind::type_check,
                   PassKind::gene_composition, PassKind::ir_gen,
                   PassKind::ir_validate, PassKind::ir_optimize,
                   PassKind::canonicalize, PassKind::canonical_validate,
                   PassKind::sprite_ir_lower, PassKind::seed_lower};
    } else {
        targets = opts.stop_after;
    }

    pm.run_passes(ctx, targets);

    if (opts.emit_json && !ctx.has_fatal_errors()) {
        auto json = IrSerializer::serialize(ctx.ir);
        if (!opts.output_dir.empty()) {
            auto out_path = opts.output_dir / (ctx.ast ? ctx.ast->name + ".json" : "output.json");
            std::ofstream ofs(out_path);
            if (ofs) ofs << json;
        }
    }
    return ctx.diagnostics;
}

void Cli::print_help() {
    std::cout
        << "Usage: gsplc [options] <input-files>\n"
        << "Options:\n"
        << "  -h, --help              Print this help\n"
        << "  -v, --version           Print version\n"
        << "  -o, --output <dir>      Output directory\n"
        << "  -I, --source-root <dir> Add source root\n"
        << "  --json                  Emit JSON output\n"
        << "  --emit-ir               Emit IR output\n"
        << "  --validate-only         Validate only (no output)\n"
        << "  --verbose               Verbose output\n"
        << "  --deterministic         Use deterministic entropy seed\n"
        << "  --model-id <id>         Model identity\n"
        << "  --stop-after=<phase>    Stop after specific phase\n";
}

void Cli::print_version() {
    std::cout << "gsplc 1.0.0 - GSPL Sprite Compiler\n";
}

void Cli::print_diagnostic(Diagnostic const& diag, SourceManager const&) {
    std::cerr << "[" << static_cast<int>(diag.severity) << "] "
              << diag.message
              << " (code: " << static_cast<std::uint32_t>(diag.code) << ")\n";
}

} // namespace gspl
