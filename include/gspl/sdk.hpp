#pragma once
#include "gspl/cli.hpp"
#include "gspl/diagnostics.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/ast.hpp"
#include "gspl/parser.hpp"
#include "gspl/modules.hpp"
#include "gspl/types.hpp"
#include "gspl/expressions.hpp"
#include "gspl/genes.hpp"
#include "gspl/ir.hpp"
#include "gspl/passes.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
#include "gspl/cache.hpp"
#include "gspl/provider.hpp"
#include "gspl/legacy.hpp"
#include "gspl/utf8.hpp"
#include <filesystem>
#include <memory>

namespace gspl {

class GsplContext {
public:
    GsplContext();
    ~GsplContext();

    GsplContext(GsplContext const&) = delete;
    GsplContext& operator=(GsplContext const&) = delete;
    GsplContext(GsplContext&&) = default;
    GsplContext& operator=(GsplContext&&) = default;

    SourceManager& sources() { return ctx_.sources; }
    ExpressionConfig& expr_config() { return ctx_.expr_config; }
    PassManager& pass_manager() { return pm_; }
    CompilationContext& compilation_context() { return ctx_; }

    DiagnosticResult compile_file(std::filesystem::path const& path);
    DiagnosticResult compile_source(SourceBuffer source);
    DiagnosticResult run_passes(std::vector<PassKind> targets);

    bool has_fatal_errors() const { return ctx_.has_fatal_errors(); }
    DiagnosticResult const& diagnostics() const { return ctx_.diagnostics; }

    static std::string version() { return "1.0.0"; }

private:
    CompilationContext ctx_;
    PassManager pm_;
};

}
