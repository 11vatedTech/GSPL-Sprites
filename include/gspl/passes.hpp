#pragma once
#include "gspl/ir.hpp"
#include "gspl/ast.hpp"
#include "gspl/expressions.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace gspl {

enum class PassKind : std::uint8_t {
    lex, parse, module_resolve, name_resolve, type_check,
    gene_composition, ir_gen, ir_validate, ir_optimize,
    representation_plan, runtime_plan, package_plan, emit
};

struct PassDescriptor {
    PassKind kind{};
    std::string name;
    std::vector<PassKind> dependencies;
    bool mandatory{true};
    std::uint32_t version{1};
};

class CompilationContext {
public:
    SourceManager sources;
    std::vector<Token> tokens;
    std::unique_ptr<ModuleDecl> ast;
    SpriteIr ir;
    DiagnosticResult diagnostics;
    ExpressionConfig expr_config;
    std::unordered_map<PassKind, PassDescriptor> pass_registry;
    void reset();
    bool has_fatal_errors() const;
};

class CompilerPass {
public:
    explicit CompilerPass(PassKind kind) : kind_(kind) {}
    virtual ~CompilerPass() = default;
    virtual DiagnosticResult execute(CompilationContext& ctx) = 0;
    PassKind kind() const { return kind_; }
private:
    PassKind kind_{};
};

class PassManager {
public:
    DiagnosticResult register_pass(std::unique_ptr<CompilerPass> pass);
    DiagnosticResult run_passes(CompilationContext& ctx, std::vector<PassKind> const& target_passes);
    std::vector<PassKind> topo_sort(std::vector<PassKind> const& targets) const;
private:
    std::unordered_map<PassKind, std::unique_ptr<CompilerPass>> passes_;
    std::unordered_map<PassKind, PassDescriptor> descriptors_;
    std::unordered_map<PassKind, bool> completed_;
};

class LexPhase final : public CompilerPass {
public:
    LexPhase() : CompilerPass(PassKind::lex) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class ParsePhase final : public CompilerPass {
public:
    ParsePhase() : CompilerPass(PassKind::parse) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class ModuleResolvePhase final : public CompilerPass {
public:
    ModuleResolvePhase() : CompilerPass(PassKind::module_resolve) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class NameResolvePhase final : public CompilerPass {
public:
    NameResolvePhase() : CompilerPass(PassKind::name_resolve) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class TypeCheckPhase final : public CompilerPass {
public:
    TypeCheckPhase() : CompilerPass(PassKind::type_check) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class GeneCompositionPhase final : public CompilerPass {
public:
    GeneCompositionPhase() : CompilerPass(PassKind::gene_composition) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class IrGenPhase final : public CompilerPass {
public:
    IrGenPhase() : CompilerPass(PassKind::ir_gen) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class IrValidatePhase final : public CompilerPass {
public:
    IrValidatePhase() : CompilerPass(PassKind::ir_validate) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

class IrOptimizePhase final : public CompilerPass {
public:
    IrOptimizePhase() : CompilerPass(PassKind::ir_optimize) {}
    DiagnosticResult execute(CompilationContext& ctx) override;
};

} // namespace gspl
