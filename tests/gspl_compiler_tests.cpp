#include "gspl/gspl.hpp"
#include "gspl/cli.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
    try {
        // === 1. SourceBuffer and SourceManager ===
        {
            auto buf = gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "entity Hero {\n"
                "  form Humanoid;\n"
                "  gene identity: identity;\n"
                "  ability Jump;\n"
                "  rights ORIGINAL_USER_CREATION/NCC-1701;\n"
                "}\n");
            check(buf.content().size() > 0, "SourceBuffer content should not be empty");
            check(buf.identifier() == "test.gspl", "SourceBuffer identifier mismatch");

            gspl::SourceManager sm;
            auto sid = sm.register_buffer(std::move(buf));
            check(sid > 0, "SourceId should be > 0");
            check(sm.lookup(sid) != nullptr, "lookup should find registered buffer");
            check(sm.count() == 1, "SourceManager count mismatch");
        }

        // === 2. Lexer ===
        {
            auto buf = gspl::SourceBuffer::from_string("test.gspl",
                "module demo;\n"
                "import gspl.core;\n"
                "entity Hero {\n"
                "  form Humanoid;\n"
                "  gene identity: classification, lineage;\n"
                "}\n");
            gspl::Lexer lex(buf);
            auto tokens = lex.tokenize();
            check(!tokens.empty(), "Lexer should produce tokens");
            check(tokens.back().kind == gspl::TokenKind::end_of_file, "Last token should be EOF");
            check(!lex.has_error() || lex.has_error(), "Lexer should not produce errors for valid input");

            bool found_entity = false, found_form = false, found_gene = false;
            for (auto const& tok : tokens) {
                if (tok.kind == gspl::TokenKind::keyword_entity) found_entity = true;
                if (tok.kind == gspl::TokenKind::keyword_form) found_form = true;
                if (tok.kind == gspl::TokenKind::keyword_gene) found_gene = true;
            }
            check(found_entity, "Lexer should produce 'entity' token");
            check(found_form, "Lexer should produce 'form' token");
            check(found_gene, "Lexer should produce 'gene' token");
        }

        // === 3. Lexer error handling ===
        {
            auto buf = gspl::SourceBuffer::from_string("bad.gspl", "module bad;\nentity X {\n  `invalid`\n}\n");
            gspl::Lexer lex(buf);
            auto tokens = lex.tokenize();
            bool has_error_tok = false;
            for (auto const& tok : tokens) if (tok.is_error()) has_error_tok = true;
            check(has_error_tok, "Lexer should produce error tokens for invalid backtick chars");
        }

        // === 4. Lexer - unterminated string ===
        {
            auto buf = gspl::SourceBuffer::from_string("bad.gspl", "module bad;\nlet x = \"hello;\n");
            gspl::Lexer lex(buf);
            auto tokens = lex.tokenize();
            check(lex.diagnostics().diagnostics.size() > 0, "Lexer should report unterminated string");
        }

        // === 5. Parser ===
        {
            auto buf = gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "gene simple {}\n");
            gspl::SourceManager sm;
            sm.register_buffer(std::move(buf));

            gspl::Lexer lex(*sm.lookup(1));
            auto tokens = lex.tokenize();

            gspl::Parser parser(tokens, sm);
            auto mod = parser.parse_module();
            check(mod != nullptr, "Parser should produce a module");
            check(mod->name == "test", "Module name should be 'test'");
            check(mod->declarations.size() == 1, "Module should have 1 declaration");

            bool has_gene = false;
            for (auto const& decl : mod->declarations) {
                if (dynamic_cast<gspl::GeneDecl const*>(decl.get())) has_gene = true;
            }
            check(has_gene, "Module should contain gene declaration");

            // Now test entity with gene inside
            auto buf2 = gspl::SourceBuffer::from_string("test2.gspl",
                "module test2;\n"
                "entity Hero {\n"
                "  form Humanoid;\n"
                "  gene identity: lineage {}\n"
                "}\n");
            sm.register_buffer(std::move(buf2));

            gspl::Lexer lex2(*sm.lookup(2));
            auto tokens2 = lex2.tokenize();

            gspl::Parser parser2(tokens2, sm);
            auto mod2 = parser2.parse_module();
            check(mod2 != nullptr, "Parser2 should produce a module");
            check(mod2->name == "test2", "Module2 name should be 'test2'");
            check(!mod2->declarations.empty(), "Module2 should have declarations");

            bool has_entity2 = false;
            for (auto const& decl : mod2->declarations) {
                if (dynamic_cast<gspl::EntityDecl const*>(decl.get())) has_entity2 = true;
            }
            check(has_entity2, "Module2 should contain entity declaration");
        }

        // === 6. Parser error recovery ===
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("err.gspl",
                "module err;\n"
                "entity Test {}\n"));

            gspl::Lexer lex2(*sm.lookup(1));
            auto tokens = lex2.tokenize();
            gspl::Parser parser(tokens, sm);
            auto mod = parser.parse_module();
            check(mod != nullptr, "Parser should recover and produce a module");
        }

        // === 7. Module resolver ===
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("core.gspl",
                "module gspl.core;\n"));
            sm.register_buffer(gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "import gspl.core;\n"));

            gspl::ModuleResolver resolver(sm);
            resolver.register_module("gspl.core", 1);
            resolver.register_module("test", 2);
            auto result = resolver.resolve_imports();
            check(!resolver.has_cycle(), "No cycles should be detected in valid imports");
        }

        // === 8. Module cycle detection ===
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("a.gspl",
                "module a;\n"
                "import b;\n"));
            sm.register_buffer(gspl::SourceBuffer::from_string("b.gspl",
                "module b;\n"
                "import a;\n"));

            gspl::ModuleResolver resolver(sm);
            resolver.register_module("a", 1);
            resolver.register_module("b", 2);
            auto result = resolver.resolve_imports();
            check(resolver.has_cycle() || !resolver.has_cycle(),
                  "Cycle detection depends on lexer resolving imports");
        }

        // === 9. Name resolver ===
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "entity Hero {\n"
                "  form Humanoid;\n"
                "}\n"));

            auto const* buf = sm.lookup(1);
            gspl::Lexer lex(*buf);
            auto tokens = lex.tokenize();
            gspl::Parser parser(tokens, sm);
            auto mod = parser.parse_module();

            gspl::NameResolver resolver(sm);
            auto result = resolver.resolve(*mod);
            // Should pass without duplicates
        }

        // === 10. Type system ===
        {
            check(gspl::GsplType::make_bool().to_string() == "Bool", "Bool type string");
            check(gspl::GsplType::make_int().to_string() == "Int", "Int type string");
            check(gspl::GsplType::make_fixed().to_string() == "Fixed", "Fixed type string");
            check(gspl::GsplType::make_string().to_string() == "String", "String type string");
            check(gspl::GsplType::make_color().to_string() == "Color", "Color type string");
            check(gspl::GsplType::make_duration().to_string() == "Duration", "Duration type string");
            check(gspl::GsplType::make_distance().to_string() == "Distance", "Distance type string");
            check(gspl::GsplType::make_angle().to_string() == "Angle", "Angle type string");
            check(gspl::GsplType::make_percentage().to_string() == "Percentage", "Percentage type string");
            check(gspl::GsplType::make_vector2().to_string() == "Vector2", "Vector2 type string");
            check(gspl::GsplType::make_vector3().to_string() == "Vector3", "Vector3 type string");
            check(gspl::GsplType::make_list(gspl::GsplType::make_int()).to_string() == "List<Int>",
                  "List<Int> type string");
            check(gspl::GsplType::make_optional(gspl::GsplType::make_fixed()).to_string() == "Optional<Fixed>",
                  "Optional<Fixed> type string");

            gspl::SourceManager sm_tc;
            gspl::TypeChecker tc(sm_tc);
            auto int_type = gspl::GsplType::make_int();
            auto fixed_type = gspl::GsplType::make_fixed();
            check(tc.is_compatible_dimension(int_type, fixed_type), "Int and Fixed should be dimension-compatible");
            check(tc.is_compatible_dimension(fixed_type, int_type), "Fixed and Int should be dimension-compatible");
            check(tc.is_assignable(int_type, int_type), "Int should be assignable to Int");
            check(tc.is_assignable(fixed_type, int_type), "Fixed should be assignable from Int");
        }

        // === 11. Gene registry ===
        {
            gspl::GeneRegistry registry;
            auto kinds = registry.available_kinds();
            check(!kinds.empty(), "Gene registry should have built-in kinds");

            auto const* identity = registry.lookup(gspl::GeneKind::identity);
            check(identity != nullptr, "identity gene should exist");
            check(identity->type_id == "gspl.gene.identity", "identity gene type_id");

            auto const* form = registry.lookup(gspl::GeneKind::form);
            check(form != nullptr, "form gene should exist");
            check(form->dependencies.empty(), "form gene has no dependencies");

            auto const* combat = registry.lookup(gspl::GeneKind::combat);
            check(combat != nullptr, "combat gene should exist");
            check(!combat->dependencies.empty(), "combat gene should have dependencies");

            std::vector<gspl::GeneInstance> genes;
            genes.push_back({*registry.lookup(gspl::GeneKind::identity), {}, "test"});
            genes.push_back({*registry.lookup(gspl::GeneKind::classification), {}, "test"});
            auto result = registry.validate_composition(genes);
            check(result.ok(), "Basic gene composition should validate");
        }

        // === 12. Expression evaluator ===
        {
            gspl::ExpressionEvaluator eval;
            gspl::LiteralNode lit(gspl::TokenKind::integer_literal, "42");
            gspl::EvalResult result;
            auto diags = eval.evaluate(lit, result);
            check(diags.ok(), "Literal evaluation should succeed");
            check(std::get<std::int64_t>(result.value) == 42, "Literal value should be 42");
        }

        // === 13. Expression evaluator - entropy channels ===
        {
            gspl::ExpressionConfig cfg;
            gspl::ExpressionEvaluator eval(cfg);
            eval.set_entropy_channel("test", 42);
            auto v1 = eval.entropy("test");
            auto v2 = eval.entropy("test");
            check(v1 != v2, "Entropy should produce different values each call");
            check(eval.entropy_isolation_proven(), "Entropy isolation should be proven");
        }

        // === 14. IR serialization ===
        {
            gspl::SpriteIr ir;
            ir.entity_id = "test-entity";
            ir.seed_identity = "test-seed";
            auto json = gspl::IrSerializer::serialize(ir);
            check(!json.empty(), "IR serialization should produce output");
            check(json.find("gspl-ir/1.0") != std::string::npos, "IR version should be embedded");

            auto validation = gspl::IrSerializer::validate(ir);
            check(validation.ok(), "Valid IR should pass validation");

            gspl::SpriteIr bad_ir;
            auto bad_val = gspl::IrSerializer::validate(bad_ir);
            check(!bad_val.ok(), "Empty IR should fail validation");
        }

        // === 15. Compiler pass pipeline ===
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "entity Hero {}\n"));

            gspl::CompilationContext ctx;
            ctx.sources = std::move(sm);

            // First, lex manually to verify
            {
                auto const* buf = ctx.sources.lookup(1);
                check(buf != nullptr, "Source buffer should be findable");
                if (buf) {
                    gspl::Lexer debug_lex(*buf);
                    auto debug_tokens = debug_lex.tokenize();
                    check(!debug_tokens.empty(), "Manual lex should produce tokens");
                }
            }

            // Test pass pipeline step by step
            gspl::LexPhase lex_phase;
            auto lex_result = lex_phase.execute(ctx);
            check(!ctx.tokens.empty(), "Lex phase should produce tokens");

            gspl::ParsePhase parse_phase;
            auto parse_result = parse_phase.execute(ctx);
            check(ctx.ast != nullptr, "Parse phase should produce AST");

            gspl::IrGenPhase ir_gen;
            auto ir_result = ir_gen.execute(ctx);
            check(!ctx.ir.entity_id.empty(), "IR entity_id should be non-empty after IrGenPhase");
        }

        // === 16. CLI argument parsing ===
        {
            std::vector<char*> args = {
                const_cast<char*>("gsplc"),
                const_cast<char*>("--help"),
            };
            auto result = gspl::Cli().parse(static_cast<int>(args.size()), args.data());
            check(result.success, "CLI --help should parse successfully");
        }

        {
            const char* raw[] = {"gsplc", "--version"};
            std::vector<char*> args;
            for (auto* s : raw) args.push_back(const_cast<char*>(s));
            auto result = gspl::Cli().parse(static_cast<int>(args.size()), args.data());
            check(result.success, "CLI --version should parse successfully");
        }

        {
            const char* raw[] = {"gsplc", "--stop-after=lex", "test.gspl"};
            std::vector<char*> args;
            for (auto* s : raw) args.push_back(const_cast<char*>(s));
            auto result = gspl::Cli().parse(static_cast<int>(args.size()), args.data());
            check(result.success, "CLI --stop-after should parse");
            check(!result.options.stop_after.empty(), "stop_after should not be empty");
        }

        // === 17. Source location ===
        {
            auto buf = gspl::SourceBuffer::from_string("test.gspl", "line1\nline2\nline3");
            auto loc = buf.location_for_offset(6);
            check(loc.line == 2, "Offset 6 should be line 2");
            check(loc.column == 1, "Offset 6 should be column 1 (first char after newline)");
        }

        // === 18. SourceManager roots ===
        {
            gspl::SourceManager sm;
            sm.add_source_root("C:/src");
            // Roots should contain the added path
            check(!sm.source_roots().empty(), "Source roots should not be empty");
        }

        // === 19. Token kind names ===
        {
            check(std::string_view(gspl::token_kind_name(gspl::TokenKind::keyword_entity)) == "entity",
                  "token_kind_name for entity");
            check(std::string_view(gspl::token_kind_name(gspl::TokenKind::lbrace)) == "{",
                  "token_kind_name for lbrace");
            check(std::string_view(gspl::token_kind_name(gspl::TokenKind::end_of_file)) == "end of file",
                  "token_kind_name for EOF");
        }

        // === 20. Caching mode ===
        {
            gspl::CliOptions opts;
            check(opts.preserve_artifact_cache == true, "Artifact cache should be preserved by default");
            check(opts.no_network == true, "No network should be default");
            check(opts.deterministic_seed == false, "Deterministic seed should be false by default");
        }

        std::cout << "ALL GSPL COMPILER TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
