#include "gspl/sdk.hpp"
#include "gspl/lowering.hpp"

namespace gspl {

GsplContext::GsplContext() {
    ctx_.expr_config.deterministic_seed = true;
    ctx_.expr_config.deterministic_entropy = 42;
}

GsplContext::~GsplContext() = default;

void register_all_passes(PassManager& pm) {
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
}

DiagnosticResult GsplContext::compile_file(std::filesystem::path const& path) {
    auto buf = SourceBuffer::from_file(path);
    return compile_source(std::move(buf));
}

DiagnosticResult GsplContext::compile_source(SourceBuffer source) {
    ctx_.sources.register_buffer(std::move(source));
    register_all_passes(pm_);
    std::vector<PassKind> targets = {
        PassKind::lex, PassKind::parse, PassKind::module_resolve,
        PassKind::name_resolve, PassKind::type_check,
        PassKind::gene_composition, PassKind::ir_gen,
        PassKind::ir_validate, PassKind::ir_optimize,
        PassKind::canonicalize, PassKind::canonical_validate,
        PassKind::sprite_ir_lower, PassKind::seed_lower
    };
    return run_passes(targets);
}

DiagnosticResult GsplContext::run_passes(std::vector<PassKind> targets) {
    pm_.run_passes(ctx_, targets);
    return ctx_.diagnostics;
}

}
