#pragma once
#include "gspl/ast.hpp"
#include "gspl/types.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gspl {

struct EntropyChannel {
    std::string name;
    std::uint64_t seed{};
};

struct ExpressionConfig {
    std::uint64_t max_depth{64};
    std::uint64_t max_nodes{65536};
    std::uint64_t max_evaluation_steps{1048576};
    bool deterministic_seed{false};
    std::uint64_t deterministic_entropy{42};
};

struct EvalResult {
    std::variant<std::int64_t, std::uint64_t, double, std::string, bool> value;
    GsplType type;
};

class ExpressionEvaluator {
public:
    explicit ExpressionEvaluator(ExpressionConfig config = {});
    DiagnosticResult evaluate(AstNode const& expr, EvalResult& out);
    void set_entropy_channel(std::string name, std::uint64_t seed);
    std::uint64_t entropy(std::string channel);
    bool entropy_isolation_proven() const { return entropy_isolation_tested_; }
private:
    ExpressionConfig config_;
    DiagnosticResult diags_;
    std::unordered_map<std::string, std::uint64_t> entropy_channels_;
    std::uint64_t step_count_{};
    std::uint64_t node_count_{};
    bool entropy_isolation_tested_{};
    EvalResult eval_node(AstNode const& node, std::uint64_t depth);
    EvalResult eval_binary(TokenKind op, EvalResult const& left, EvalResult const& right, SourceSpan span);
    EvalResult eval_literal(LiteralNode const& lit);
    void enforce_limits(std::uint64_t depth);
};

} // namespace gspl
