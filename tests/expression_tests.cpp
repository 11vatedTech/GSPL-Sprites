#include "gspl/gspl.hpp"
#include "gspl/expressions.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. Integer literal evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "42");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Integer eval should succeed");
            check(std::holds_alternative<std::int64_t>(result.value), "Result should be int64");
            check(std::get<std::int64_t>(result.value) == 42, "Value should be 42");
            check(result.type.base == gspl::BaseType::Int, "Type should be Int");
        }

        // ---- 2. Unsigned literal evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::unsigned_literal, "100");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Unsigned eval should succeed");
            check(std::holds_alternative<std::uint64_t>(result.value), "Result should be uint64");
            check(std::get<std::uint64_t>(result.value) == 100, "Value should be 100");
            check(result.type.base == gspl::BaseType::UInt, "Type should be UInt");
        }

        // ---- 3. Fixed literal evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::fixed_literal, "3.14");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Fixed eval should succeed");
            check(std::holds_alternative<double>(result.value), "Result should be double");
            check(result.type.base == gspl::BaseType::Fixed, "Type should be Fixed");
        }

        // ---- 4. String literal evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::string_literal, "\"hello\"");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "String eval should succeed");
            check(std::holds_alternative<std::string>(result.value), "Result should be string");
            check(std::get<std::string>(result.value) == "hello", "Value should be 'hello' (stripped quotes)");
            check(result.type.base == gspl::BaseType::String, "Type should be String");
        }

        // ---- 5. Boolean literal evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::keyword_true, "true");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Bool true eval should succeed");
            check(std::holds_alternative<bool>(result.value), "Result should be bool");
            check(std::get<bool>(result.value) == true, "Value should be true");
            check(result.type.base == gspl::BaseType::Bool, "Type should be Bool");
        }

        // ---- 6. Boolean false evaluation ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::keyword_false, "false");
            gspl::EvalResult result;
            eval.evaluate(lit, result);
            check(std::get<bool>(result.value) == false, "Value should be false");
        }

        // ---- 7. Entropy channel ----
        {
            gspl::ExpressionEvaluator eval;
            eval.set_entropy_channel("test", 42);
            auto v1 = eval.entropy("test");
            check(v1 > 0, "Entropy should produce non-zero value");
            auto v2 = eval.entropy("test");
            check(v1 != v2, "Entropy should produce different values on successive calls");
            check(eval.entropy_isolation_proven(), "Entropy isolation should be proven");
        }

        // ---- 8. Different entropy channels produce different streams ----
        {
            gspl::ExpressionEvaluator eval;
            auto a1 = eval.entropy("channel_a");
            eval.set_entropy_channel("channel_b", 42);
            auto b1 = eval.entropy("channel_b");
            check(a1 != b1, "Different channels should produce different entropy");
        }

        // ---- 9. Expression depth limit enforcement ----
        {
            gspl::ExpressionConfig cfg;
            cfg.max_depth = 3;
            gspl::ExpressionEvaluator eval(cfg);
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "1");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            bool has_limit_err = false;
            for (auto const& d : diags.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED) has_limit_err = true;
            }
            check(!has_limit_err, "Shallow expression should pass depth limit");
        }

        // ---- 10. Evaluation step counting ----
        {
            gspl::ExpressionConfig cfg;
            cfg.max_evaluation_steps = 5;
            gspl::ExpressionEvaluator eval(cfg);
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "99");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Expression within step limit should pass");
        }

        // ---- 11. Deterministic seed mode ----
        {
            gspl::ExpressionConfig cfg;
            cfg.deterministic_seed = true;
            cfg.deterministic_entropy = 42;
            gspl::ExpressionEvaluator eval(cfg);
            eval.set_entropy_channel("det", 42);
            auto v1 = eval.entropy("det");

            gspl::ExpressionEvaluator eval2(cfg);
            eval2.set_entropy_channel("det", 42);
            auto v2 = eval2.entropy("det");
            check(v1 == v2, "Deterministic entropy should produce same value with same seed");
        }

        // ---- 12. Large integer literal ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "9223372036854775807");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "INT64_MAX should parse");
            check(std::get<std::int64_t>(result.value) == 9223372036854775807LL, "Value should be INT64_MAX");
        }

        // ---- 13. Zero literal ----
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "0");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Zero literal should evaluate without error");
        }

        std::cout << "ALL EXPRESSION TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
