#include "gspl/passes.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"
#include "gspl/modules.hpp"
#include "gspl/types.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <deque>
#include <functional>
#include <ranges>
#include <set>
#include <initializer_list>
#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace gspl {
namespace {

std::string strip_gene_literal(std::string value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

GeneValue lower_gene_value(LiteralNode const& literal, DiagnosticResult& diagnostics, SourceSpan span) {
    auto text = strip_gene_literal(literal.value);
    switch (literal.literal_kind) {
    case TokenKind::boolean_literal:
    case TokenKind::keyword_true:
    case TokenKind::keyword_false:
        if (text == "true") return true;
        if (text == "false") return false;
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                              "Invalid boolean gene value: " + literal.value, span);
        return false;
    case TokenKind::integer_literal: {
        std::int64_t value{};
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec == std::errc{} && ptr == text.data() + text.size()) return value;
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                              "Invalid signed integer gene value: " + literal.value, span);
        return std::int64_t{};
    }
    case TokenKind::unsigned_literal: {
        std::uint64_t value{};
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec == std::errc{} && ptr == text.data() + text.size()) return value;
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                              "Invalid unsigned integer gene value: " + literal.value, span);
        return std::uint64_t{};
    }
    case TokenKind::fixed_literal:
    case TokenKind::duration_literal:
    case TokenKind::distance_literal:
    case TokenKind::angle_literal:
    case TokenKind::percentage_literal: {
        try {
            std::size_t consumed{};
            const double value = std::stod(text, &consumed);
            if (consumed == text.size()) return value;
        } catch (std::exception const&) {}
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                              "Invalid numeric gene value: " + literal.value, span);
        return 0.0;
    }
    case TokenKind::string_literal:
    case TokenKind::identifier:
    case TokenKind::color_literal:
    default:
        return text;
    }
}

bool looks_like_hex_color(std::string_view value) {
    if (value.size() != 7 || value.front() != '#') return false;
    return std::ranges::all_of(value.substr(1), [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c)) != 0;
    });
}

bool is_string_like(GeneValue const& value) {
    return std::holds_alternative<std::string>(value);
}

bool is_integer_like(GeneValue const& value) {
    return std::holds_alternative<std::int64_t>(value) || std::holds_alternative<std::uint64_t>(value);
}

bool is_number_like(GeneValue const& value) {
    return is_integer_like(value) || std::holds_alternative<double>(value);
}

void validate_known_gene_payload(GeneDecl const& gene, GeneInstance const& instance, DiagnosticResult& diagnostics) {
    auto reject = [&](std::string const& message) {
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE, message, gene.span);
    };
    auto require_string = [&](std::string const& key) {
        auto found = instance.values.find(key);
        if (found != instance.values.end() && !is_string_like(found->second)) {
            reject("Gene '" + gene.name + "' field '" + key + "' must be a string or identifier");
        }
    };
    auto require_color = [&](std::string const& key) {
        auto found = instance.values.find(key);
        if (found == instance.values.end()) return;
        if (!is_string_like(found->second) || !looks_like_hex_color(gene_value_to_string(found->second))) {
            reject("Gene '" + gene.name + "' field '" + key + "' must be a #RRGGBB color");
        }
    };
    auto require_number = [&](std::string const& key) {
        auto found = instance.values.find(key);
        if (found != instance.values.end() && !is_number_like(found->second)) {
            reject("Gene '" + gene.name + "' field '" + key + "' must be numeric");
        }
    };
    auto reject_unknown_except = [&](std::initializer_list<std::string_view> allowed) {
        for (auto const& [key, value] : instance.values) {
            static_cast<void>(value);
            const bool ok = std::ranges::any_of(allowed, [&](std::string_view allowed_key) { return key == allowed_key; });
            if (!ok) reject("Gene '" + gene.name + "' has unsupported field '" + key + "'");
        }
    };

    switch (instance.descriptor.kind) {
    case GeneKind::identity:
        reject_unknown_except({"stable_id", "name"});
        require_string("stable_id");
        require_string("name");
        break;
    case GeneKind::classification:
        reject_unknown_except({"taxonomy", "classification"});
        require_string("taxonomy");
        require_string("classification");
        break;
    case GeneKind::appearance:
        reject_unknown_except({"primary_color", "accent_color", "storm_primary_color", "storm_accent_color", "emissive_color", "aura_color"});
        require_color("primary_color");
        require_color("accent_color");
        require_color("storm_primary_color");
        require_color("storm_accent_color");
        require_color("emissive_color");
        require_color("aura_color");
        break;
    case GeneKind::rights:
        reject_unknown_except({"classification", "allow_export"});
        require_string("classification");
        break;
    case GeneKind::provenance:
        reject_unknown_except({"hash", "source"});
        require_string("hash");
        require_string("source");
        break;
    case GeneKind::morphology:
        reject_unknown_except({"part", "parent", "x", "y", "z", "size_x", "size_y", "size_z", "color", "rotation_degrees", "emissive", "electrical_marking"});
        require_string("part");
        require_string("parent");
        require_number("x");
        require_number("y");
        require_number("z");
        require_number("size_x");
        require_number("size_y");
        require_number("size_z");
        require_color("color");
        require_number("rotation_degrees");
        break;
    case GeneKind::optimization: {
        reject_unknown_except({"entropy_root"});
        auto found = instance.values.find("entropy_root");
        if (found != instance.values.end() && !is_integer_like(found->second)) {
            reject("Gene 'optimization' field 'entropy_root' must be an integer");
        }
        break;
    }
    default:
        break;
    }
}

GeneKind gene_kind_from_name(std::string_view name, bool& known) {
    static const std::unordered_map<std::string_view, GeneKind> kinds{
        {"identity", GeneKind::identity}, {"classification", GeneKind::classification},
        {"lineage", GeneKind::lineage}, {"form", GeneKind::form},
        {"transformation", GeneKind::transformation}, {"morphology", GeneKind::morphology},
        {"anatomy", GeneKind::anatomy}, {"structure", GeneKind::structure},
        {"proportion", GeneKind::proportion}, {"appearance", GeneKind::appearance},
        {"palette", GeneKind::palette}, {"material", GeneKind::material},
        {"surface", GeneKind::surface}, {"equipment", GeneKind::equipment},
        {"motion", GeneKind::motion}, {"locomotion", GeneKind::locomotion},
        {"animation", GeneKind::animation}, {"expression", GeneKind::expression},
        {"behavior", GeneKind::behavior}, {"perception", GeneKind::perception},
        {"memory", GeneKind::memory}, {"emotion", GeneKind::emotion},
        {"ability", GeneKind::ability}, {"combat", GeneKind::combat},
        {"projectile", GeneKind::projectile}, {"effect", GeneKind::effect},
        {"interaction", GeneKind::interaction}, {"physics", GeneKind::physics},
        {"collision", GeneKind::collision}, {"audio_event", GeneKind::audio_event},
        {"audio", GeneKind::audio_event}, {"projection", GeneKind::projection},
        {"optimization", GeneKind::optimization}, {"target", GeneKind::target},
        {"rights", GeneKind::rights}, {"provenance", GeneKind::provenance},
    };
    const auto found = kinds.find(name);
    known = found != kinds.end();
    return known ? found->second : GeneKind::identity;
}

void collect_gene_declarations(AstNode const& node, std::vector<GeneDecl const*>& genes) {
    if (auto const* gene = dynamic_cast<GeneDecl const*>(&node)) {
        genes.push_back(gene);
        return;
    }
    if (auto const* module = dynamic_cast<ModuleDecl const*>(&node)) {
        for (auto const& decl : module->declarations) collect_gene_declarations(*decl, genes);
        return;
    }
    if (auto const* entity = dynamic_cast<EntityDecl const*>(&node)) {
        for (auto const& child : entity->body) collect_gene_declarations(*child, genes);
    }
}

GeneInstance lower_gene_instance(GeneDecl const& gene, GeneRegistry const& registry, DiagnosticResult& diagnostics) {
    bool known = false;
    const auto kind = gene_kind_from_name(gene.name, known);
    if (!known) {
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_UNKNOWN,
                              "Unknown gene kind: " + gene.name, gene.span);
        return {};
    }
    auto const* descriptor = registry.lookup(kind);
    if (descriptor == nullptr) {
        diagnostics.add_error(DiagnosticCode::GSPL_GENE_UNKNOWN,
                              "Unregistered gene kind: " + gene.name, gene.span);
        return {};
    }
    GeneInstance instance;
    instance.descriptor = *descriptor;
    instance.source_module = gene.name;
    for (auto const& child : gene.body) {
        auto const* attr = dynamic_cast<AttributeNode const*>(child.get());
        if (attr == nullptr || attr->value == nullptr) continue;
        auto const* literal = dynamic_cast<LiteralNode const*>(attr->value.get());
        if (literal == nullptr) {
            diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                  "Gene attribute is not a literal: " + attr->key, attr->span);
            continue;
        }
        instance.values[attr->key] = lower_gene_value(*literal, diagnostics, attr->span);
    }
    validate_known_gene_payload(gene, instance, diagnostics);
    return instance;
}

} // namespace

void CompilationContext::reset() {
    tokens.clear();
    ast.reset();
    ir = {};
    canonical = {};
    composed_genes.clear();
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

void PassManager::reset_completion() {
    completed_.clear();
}

DiagnosticResult PassManager::run_passes(CompilationContext& ctx,
                                          std::vector<PassKind> const& target_passes) {
    reset_completion();
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
    if (!ctx.ast) return {};
    GeneRegistry registry;
    std::vector<GeneDecl const*> declarations;
    collect_gene_declarations(*ctx.ast, declarations);
    std::vector<GeneInstance> genes;
    genes.reserve(declarations.size());
    std::set<GeneKind> non_repeatable_seen;
    for (auto const* declaration : declarations) {
        auto instance = lower_gene_instance(*declaration, registry, ctx.diagnostics);
        if (instance.descriptor.type_id.empty()) continue;
        const bool repeatable = instance.descriptor.kind == GeneKind::ability ||
            instance.descriptor.kind == GeneKind::morphology;
        if (!repeatable && !non_repeatable_seen.insert(instance.descriptor.kind).second) {
            ctx.diagnostics.add_error(DiagnosticCode::GSPL_GENE_DUPLICATE,
                                      "Duplicate non-repeatable gene declaration: " + declaration->name,
                                      declaration->span);
            continue;
        }
        genes.push_back(std::move(instance));
    }
    auto validation = registry.validate_composition(genes);
    for (auto& d : validation.diagnostics) ctx.diagnostics.add(std::move(d));
    ctx.composed_genes = registry.compose({}, std::move(genes));
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
    // Canonical-normalization pass: applies deterministic ordering to
    // collection fields so that serialized output is stable regardless
    // of insertion order. This pass does NOT alter semantic behavior.
    // Contract: behavior(before) == behavior(after).

    if (ctx.canonical.stable_id.empty()) return {}; // nothing to normalize

    auto& ce = ctx.canonical;

    // Sort forms by id
    std::sort(ce.forms.begin(), ce.forms.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort transformations by id
    std::sort(ce.transformations.begin(), ce.transformations.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort abilities by id
    std::sort(ce.abilities.begin(), ce.abilities.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });
    std::sort(ce.storm_abilities.begin(), ce.storm_abilities.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort bones by id
    std::sort(ce.bones.begin(), ce.bones.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort sockets by id
    std::sort(ce.sockets.begin(), ce.sockets.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort clips by name
    std::sort(ce.clips.begin(), ce.clips.end(),
              [](auto const& a, auto const& b) { return a.name < b.name; });

    // Sort states by name
    std::sort(ce.states.begin(), ce.states.end(),
              [](auto const& a, auto const& b) { return a.name < b.name; });

    // Sort collision shapes by id
    std::sort(ce.collision_shapes.begin(), ce.collision_shapes.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort collision windows by ability_id then shape_id
    std::sort(ce.collision_windows.begin(), ce.collision_windows.end(),
              [](auto const& a, auto const& b) {
                  if (a.ability_id != b.ability_id) return a.ability_id < b.ability_id;
                  return a.shape_id < b.shape_id;
              });

    // Sort resources by id
    std::sort(ce.resources.begin(), ce.resources.end(),
              [](auto const& a, auto const& b) { return a.id < b.id; });

    // Sort genes by kind for deterministic serialization
    std::sort(ce.genes.begin(), ce.genes.end(),
              [](auto const& a, auto const& b) {
                  return a.descriptor.kind < b.descriptor.kind;
              });

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
    auto gene_result = registry.validate_composition(ctx.composed_genes);
    auto has_errors = std::any_of(gene_result.diagnostics.begin(), gene_result.diagnostics.end(),
        [](auto const& d) { return d.severity >= DiagnosticSeverity::error; });
    if (has_errors) {
        for (auto const& gd : gene_result.diagnostics) ctx.diagnostics.add(gd);
        return ctx.diagnostics;
    }
    Canonicalizer canonicalizer(ctx.sources);
    ctx.canonical = canonicalizer.lower(*ctx.ast, ctx.composed_genes);
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
