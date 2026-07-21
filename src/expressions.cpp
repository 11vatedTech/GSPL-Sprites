#include "gspl/expressions.hpp"
#include <charconv>
#include <cmath>
#include <random>

namespace gspl {

ExpressionEvaluator::ExpressionEvaluator(ExpressionConfig config) : config_(config) {}

void ExpressionEvaluator::enforce_limits(std::uint64_t depth) {
    ++step_count_;
    ++node_count_;
    if (depth > config_.max_depth) {
        diags_.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED,
                         "Expression exceeds maximum nesting depth", {});
    }
    if (node_count_ > config_.max_nodes) {
        diags_.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED,
                         "Expression exceeds maximum node count", {});
    }
    if (step_count_ > config_.max_evaluation_steps) {
        diags_.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED,
                         "Evaluation exceeded maximum step count", {});
    }
}

EvalResult ExpressionEvaluator::eval_literal(LiteralNode const& lit) {
    switch (lit.literal_kind) {
    case TokenKind::integer_literal: {
        std::int64_t val = 0;
        std::from_chars(lit.value.data(), lit.value.data() + lit.value.size(), val);
        return {val, GsplType::make_int()};
    }
    case TokenKind::unsigned_literal: {
        std::uint64_t val = 0;
        std::from_chars(lit.value.data(), lit.value.data() + lit.value.size(), val);
        return {val, GsplType::make_uint()};
    }
    case TokenKind::fixed_literal: {
        double val = 0.0;
        std::from_chars(lit.value.data(), lit.value.data() + lit.value.size(), val);
        return {val, GsplType::make_fixed()};
    }
    case TokenKind::string_literal: {
        auto s = lit.value;
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size() - 2);
        return {s, GsplType::make_string()};
    }
    case TokenKind::keyword_true: return {true, GsplType::make_bool()};
    case TokenKind::keyword_false: return {false, GsplType::make_bool()};
    default: return {std::int64_t(0), GsplType::make_int()};
    }
}

EvalResult ExpressionEvaluator::eval_binary(TokenKind op, EvalResult const& left,
                                              EvalResult const& right, SourceSpan) {
    auto visit_numeric = [&](auto&& fn) -> EvalResult {
        return std::visit([&](auto const& l, auto const& r) -> EvalResult {
            using L = std::decay_t<decltype(l)>;
            using R = std::decay_t<decltype(r)>;
            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                auto result = fn(l, r);
                if (left.type.base == BaseType::Fixed || right.type.base == BaseType::Fixed)
                    return {static_cast<double>(result), GsplType::make_fixed()};
                return {static_cast<std::int64_t>(result), GsplType::make_int()};
            }
            return {std::int64_t(0), GsplType::make_int()};
        }, left.value, right.value);
    };
    switch (op) {
    case TokenKind::plus: return visit_numeric(std::plus<>{});
    case TokenKind::minus: return visit_numeric(std::minus<>{});
    case TokenKind::star: return visit_numeric(std::multiplies<>{});
    case TokenKind::slash: return visit_numeric(std::divides<>{});
    default: return {std::int64_t(0), GsplType::make_int()};
    }
}

EvalResult ExpressionEvaluator::eval_node(AstNode const& node, std::uint64_t depth) {
    enforce_limits(depth);
    if (auto const* lit = dynamic_cast<LiteralNode const*>(&node)) return eval_literal(*lit);
    return {std::int64_t(0), GsplType::make_int()};
}

DiagnosticResult ExpressionEvaluator::evaluate(AstNode const& expr, EvalResult& out) {
    step_count_ = 0;
    node_count_ = 0;
    out = eval_node(expr, 0);
    return diags_;
}

void ExpressionEvaluator::set_entropy_channel(std::string name, std::uint64_t seed) {
    entropy_channels_[name] = seed;
}

std::uint64_t ExpressionEvaluator::entropy(std::string channel) {
    auto& seed = entropy_channels_[channel];
    if (seed == 0) {
        seed = std::hash<std::string>{}(channel);
    }
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    entropy_isolation_tested_ = true;
    return seed >> 33;
}

} // namespace gspl
