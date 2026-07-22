#include "gspl/passes.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"
#include "gspl/modules.hpp"
#include "gspl/types.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include <deque>
#include <functional>
#include <ranges>
#include <set>

namespace gspl {

void CompilationContext::reset() {
    tokens.clear();
    ast.reset();
    diagnostics = {};
    pass_registry.clear();
}

bool CompilationContext::has_fatal_errors() const {
    return std::any_of(diagnostics.diagnostics.begin(), diagnostics.diagnostics.end(), [](auto const& d) {
        return d.severity >= DiagnosticSeverity::error;
    });
}

DiagnosticResult PassManager::register_pass(std::unique_ptr<CompilerPass> pass) {
    auto kind = pass->kind();
    if (passes_.contains(kind)) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_PASS_DUPLICATE;
        d.severity = DiagnosticSeverity::error;
        d.message = "Duplicate pass registration: " + std::to_string(static_cast<int>(kind));
        return DiagnosticResult{{d}};
    }
    PassDescriptor desc;
    desc.kind = kind;
    desc.name = std::to_string(static_cast<int>(kind));
    desc.mandatory = true;
    desc.version = 1;
    switch (kind) {
    case PassKind::lex: desc.dependencies = {}; break;
    case PassKind::parse: desc.dependencies = {PassKind::lex}; break;
    case PassKind::module_resolve: desc.dependencies = {PassKind::parse}; break;
    case PassKind::name_resolve: desc.dependencies = {PassKind::module_resolve}; break;
    case PassKind::type_check: desc.dependencies = {PassKind::name_resolve}; break;
    case PassKind::gene_composition: desc.dependencies = {PassKind::type_check}; break;
    case PassKind::ir_gen: desc.dependencies = {PassKind::gene_composition}; break;
    case PassKind::ir_validate: desc.dependencies = {PassKind::ir_gen}; break;
    case PassKind::ir_optimize: desc.dependencies = {PassKind::ir_validate}; break;
    case PassKind::canonicalize: desc.dependencies = {PassKind::name_resolve, PassKind::type_check, PassKind::gene_composition}; break;
    case PassKind::canonical_validate: desc.dependencies = {PassKind::canonicalize}; break;
    case PassKind::sprite_ir_lower: desc.dependencies = {PassKind::canonical_validate}; break;
    case PassKind::seed_lower: desc.dependencies = {PassKind::canonicalize}; break;
    default: break;
    }
    descriptors_[kind] = desc;
    passes_[kind] = std::move(pass);
    return {};
}

std::vector<PassKind> PassManager::topo_sort(std::vector<PassKind> const& targets) const {
    std::vector<PassKind> sorted;
    std::set<PassKind> visited;
    std::set<PassKind> in_stack;
    std::deque<PassKind> stack;

    std::function<bool(PassKind)> dfs = [&](PassKind kind) -> bool {
        if (visited.contains(kind)) return false;
        if (in_stack.contains(kind)) return true;
        auto it = descriptors_.find(kind);
        if (it == descriptors_.end()) return false;
        in_stack.insert(kind);
        stack.push_back(kind);
        for (auto dep : it->second.dependencies) {
            if (dfs(dep)) return true;
        }
        stack.pop_back();
        in_stack.erase(kind);
        visited.insert(kind);
        sorted.push_back(kind);
        return false;
    };
    for (auto t : targets) dfs(t);
    return sorted;
}

DiagnosticResult PassManager::run_passes(CompilationContext& ctx,
                                          std::vector<PassKind> const& target_passes) {
    auto sorted = topo_sort(target_passes);
    for (auto kind : sorted) {
        auto it = passes_.find(kind);
        if (it == passes_.end()) {
            Diagnostic d;
            d.code = DiagnosticCode::GSPL_PASS_MISSING_DEPENDENCY;
            d.severity = DiagnosticSeverity::error;
            d.message = "Pass not registered: " + std::to_string(static_cast<int>(kind));
            ctx.diagnostics.add(d);
            return ctx.diagnostics;
        }
        if (completed_[kind]) continue;
        if (ctx.has_fatal_errors()) break;
        auto result = it->second->execute(ctx);
        for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
        completed_[kind] = true;
    }
    return ctx.diagnostics;
}

DiagnosticResult LexPhase::execute(CompilationContext& ctx) {
    for (gspl::SourceId sid = 1; sid <= ctx.sources.count(); ++sid) {
        auto const* buf = ctx.sources.lookup(sid);
        if (!buf) continue;
        Lexer lex(*buf);
        auto tokens = lex.tokenize();
        for (auto& t : tokens) ctx.tokens.push_back(std::move(t));
        for (auto& d : lex.diagnostics().diagnostics) ctx.diagnostics.add(d);
    }
    return {};
}

DiagnosticResult ParsePhase::execute(CompilationContext& ctx) {
    if (ctx.tokens.empty()) {
        ctx.diagnostics.add_error(DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
                                  "No tokens to parse", {});
        return ctx.diagnostics;
    }
    Parser parser(ctx.tokens, ctx.sources);
    ctx.ast = parser.parse_module();
    for (auto& d : parser.diagnostics().diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult ModuleResolvePhase::execute(CompilationContext& ctx) {
    if (!ctx.ast) return {};
    ModuleResolver resolver(ctx.sources);
    resolver.register_module(ctx.ast->name, 0);
    auto result = resolver.resolve_imports();
    for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult NameResolvePhase::execute(CompilationContext& ctx) {
    if (!ctx.ast) return {};
    NameResolver resolver(ctx.sources);
    auto result = resolver.resolve(*ctx.ast);
    for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult TypeCheckPhase::execute(CompilationContext& ctx) {
    if (!ctx.ast) return {};
    TypeChecker checker(ctx.sources);
    auto result = checker.check_types(*ctx.ast);
    for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult GeneCompositionPhase::execute(CompilationContext& ctx) {
    (void)ctx;
    return {};
}

DiagnosticResult IrGenPhase::execute(CompilationContext& ctx) {
    ctx.ir.entity_id = ctx.ast ? ctx.ast->name : "unknown";
    ctx.ir.seed_identity = "gspl-deterministic";
    ctx.ir.entity = std::make_unique<EntityIr>();
    ctx.ir.entity->entity_id = ctx.ir.entity_id;
    return {};
}

DiagnosticResult IrValidatePhase::execute(CompilationContext& ctx) {
    auto result = IrSerializer::validate(ctx.ir);
    for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult IrOptimizePhase::execute(CompilationContext& ctx) {
    (void)ctx;
    return {};
}

DiagnosticResult CanonicalizePhase::execute(CompilationContext& ctx) {
    if (!ctx.ast) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_MODULE_UNRESOLVED;
        d.severity = DiagnosticSeverity::error;
        d.message = "Cannot canonicalize: no AST";
        return DiagnosticResult{{d}};
    }
    // Require preceding semantic passes to have completed
    if (ctx.has_fatal_errors()) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_TYPE_MISMATCH;
        d.severity = DiagnosticSeverity::error;
        d.message = "Cannot canonicalize: preceding semantic passes have errors";
        return DiagnosticResult{{d}};
    }
    GeneRegistry registry;
    auto gene_result = registry.validate_composition({});
    auto has_errors = std::any_of(gene_result.diagnostics.begin(), gene_result.diagnostics.end(),
        [](auto const& d) { return d.severity >= DiagnosticSeverity::error; });
    if (has_errors) {
        for (auto const& gd : gene_result.diagnostics) ctx.diagnostics.add(gd);
        return ctx.diagnostics;
    }
    Canonicalizer canonicalizer(ctx.sources);
    ctx.canonical = canonicalizer.lower(*ctx.ast, {});
    for (auto const& d : canonicalizer.diagnostics().diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult CanonicalValidatePhase::execute(CompilationContext& ctx) {
    CanonicalEntityValidator validator;
    auto result = validator.validate(ctx.canonical);
    for (auto& d : result.diagnostics) ctx.diagnostics.add(d);
    return {};
}

DiagnosticResult SpriteIrLowerPhase::execute(CompilationContext& ctx) {
    auto lowering_result = SpriteIrLowering::lower(ctx.canonical);
    for (auto const& ld : lowering_result.diagnostics) {
        Diagnostic diag;
        switch (ld.code) {
        case LoweringDiagnostic::Code::RIGHTS_INVALID:
            diag.code = DiagnosticCode::GSPL_RIGHTS_VIOLATION; break;
        case LoweringDiagnostic::Code::COLOR_INVALID:
            diag.code = DiagnosticCode::GSPL_TYPE_INVALID_CONVERSION; break;
        case LoweringDiagnostic::Code::SEMANTIC_LOSS:
        case LoweringDiagnostic::Code::INTERNAL_FAILURE:
            diag.code = DiagnosticCode::GSPL_TYPE_MISMATCH; break;
        default:
            diag.code = DiagnosticCode::GSPL_TYPE_MISMATCH; break;
        }
        diag.severity = ld.is_error() ? DiagnosticSeverity::error : DiagnosticSeverity::warning;
        diag.message = ld.message;
        ctx.diagnostics.add(diag);
    }
    // Store the lowered production SpriteIr into the existing CompilationContext
    // (We can't store gspl::sprites::SpriteIr directly, so we just validate it compiled)
    try {
        // Lower via SpriteSeed for compatibility and run through production compile()
        auto seed = SpriteSeedLowering::lower(ctx.canonical);
        auto validation = gspl::sprites::validate(seed);
        if (!validation.ok()) {
            for (auto& d : validation.diagnostics) {
                Diagnostic diag;
                diag.code = DiagnosticCode::GSPL_TYPE_MISMATCH;
                diag.severity = DiagnosticSeverity::error;
                diag.message = d.message;
                ctx.diagnostics.add(diag);
            }
            return ctx.diagnostics;
        }
        auto prod_ir = gspl::sprites::compile(seed);
        ctx.ir.entity_id = prod_ir.entity_id;
        ctx.ir.seed_identity = prod_ir.seed_identity;
    } catch (std::exception const& e) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_TYPE_MISMATCH;
        d.severity = DiagnosticSeverity::error;
        d.message = std::string("Production compilation failed: ") + e.what();
        return DiagnosticResult{{d}};
    }
    return {};
}

DiagnosticResult SeedLowerPhase::execute(CompilationContext& ctx) {
    auto seed = SpriteSeedLowering::lower(ctx.canonical);
    try {
        auto validation = gspl::sprites::validate(seed);
        if (!validation.ok()) {
            for (auto& d : validation.diagnostics) {
                Diagnostic diag;
                diag.code = DiagnosticCode::GSPL_TYPE_MISMATCH;
                diag.severity = DiagnosticSeverity::error;
                diag.message = d.message;
                ctx.diagnostics.add(diag);
            }
            return ctx.diagnostics;
        }
        auto prod_ir = gspl::sprites::compile(seed);
        ctx.ir.entity_id = prod_ir.entity_id;
        ctx.ir.seed_identity = prod_ir.seed_identity;
    } catch (std::exception const& e) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_TYPE_MISMATCH;
        d.severity = DiagnosticSeverity::error;
        d.message = std::string("Sprite seed compilation failed: ") + e.what();
        return DiagnosticResult{{d}};
    }
    return {};
}

} // namespace gspl
