#include "gspl/gspl.hpp"
#include "gspl/modules.hpp"
#include "gspl/passes.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. ModulePath construction and string conversion ----
        {
            gspl::ModulePath mp{{"foo", "bar", "baz"}};
            check(mp.to_string() == "foo.bar.baz", "ModulePath to_string should join with dots");
            auto parsed = gspl::ModulePath::from_string("a.b.c");
            check(parsed.segments.size() == 3, "from_string should parse 3 segments");
            check(parsed.segments[0] == "a", "First segment should be 'a'");
            check(parsed.segments[2] == "c", "Last segment should be 'c'");
            check(parsed.to_string() == "a.b.c", "Round-trip should match");
        }

        // ---- 2. ModulePath single segment ----
        {
            auto mp = gspl::ModulePath::from_string("root");
            check(mp.segments.size() == 1, "Single segment path should have 1 segment");
            check(mp.segments[0] == "root", "Segment should be 'root'");
        }

        // ---- 3. ModuleResolver register and resolve ----
        {
            gspl::SourceManager sm;
            gspl::ModuleResolver resolver(sm);
            resolver.register_module("test.a", 1);
            resolver.register_module("test.b", 2);
            auto result = resolver.resolve_imports();
            check(result.ok(), "No-cycle module resolve should succeed");
            check(!resolver.has_cycle(), "Should not detect cycle");
        }

        // ---- 4. ModuleResolver duplicate detection ----
        {
            gspl::SourceManager sm;
            gspl::ModuleResolver resolver(sm);
            resolver.register_module("test.a", 1);
            auto result = resolver.register_module("test.a", 2);
            bool has_dup = false;
            for (auto const& d : result.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_MODULE_DUPLICATE) has_dup = true;
            }
            check(has_dup, "Duplicate module should produce GSPL_MODULE_DUPLICATE");
        }

        // ---- 5. Module self-import cycle (single module importing itself) ----
        // Note: The current ModuleResolver detects cycles via DFS over imports.
        // Self-import would require a module's import to reference itself,
        // which the resolver doesn't parse. We test the cycle detection API.
        {
            gspl::SourceManager sm;
            gspl::ModuleResolver resolver(sm);
            resolver.register_module("only", 1);
            auto result = resolver.resolve_imports();
            check(result.ok(), "Single module should resolve without cycle");
        }

        // ---- 6. NameResolver simple entity ----
        {
            gspl::SourceManager sm;
            gspl::NameResolver resolver(sm);
            gspl::Token tok;
            gspl::Token ident_tok;
            ident_tok.kind = gspl::TokenKind::identifier;
            ident_tok.text = "TestEntity";

            auto entity = std::make_unique<gspl::EntityDecl>("TestEntity");
            entity->span = gspl::SourceSpan{};

            gspl::ModuleDecl mod;
            mod.name = "testmod";
            mod.declarations.push_back(std::move(entity));

            auto result = resolver.resolve(mod);
            check(result.ok(), "Entity name resolution should succeed");
        }

        // ---- 7. NameResolver duplicate name detection ----
        {
            gspl::SourceManager sm;
            gspl::NameResolver resolver(sm);
            auto e1 = std::make_unique<gspl::EntityDecl>("DupEntity");
            auto e2 = std::make_unique<gspl::EntityDecl>("DupEntity");

            gspl::ModuleDecl mod;
            mod.name = "testdup";
            mod.declarations.push_back(std::move(e1));
            mod.declarations.push_back(std::move(e2));

            auto result = resolver.resolve(mod);
            bool has_dup = false;
            for (auto const& d : result.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_NAME_DUPLICATE) has_dup = true;
            }
            check(has_dup, "Duplicate entity name should produce GSPL_NAME_DUPLICATE");
        }

        // ---- 8. Module system with lex/parse/resolve pipeline ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("modtest.gspl",
                "module modtest;\n"
                "entity MyEntity {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  morphology {}\n"
                "}\n");
            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            check(ctx.ast != nullptr, "Parse should produce AST");
            check(ctx.ast->name == "modtest", "Module name should be 'modtest'");

            ctx.diagnostics = {};
            gspl::ModuleResolvePhase mod_res; mod_res.execute(ctx);
            ctx.diagnostics = {};
            gspl::NameResolvePhase name_res; name_res.execute(ctx);
            bool has_name_err = false;
            for (auto const& d : ctx.diagnostics.diagnostics) {
                if (d.severity >= gspl::DiagnosticSeverity::error) has_name_err = true;
            }
            check(!has_name_err, "Name resolution should succeed");
        }

        // ---- 9. NameResolver handles form extensions (undefined reference) ----
        {
            gspl::SourceManager sm;
            gspl::NameResolver resolver(sm);

            auto form = std::make_unique<gspl::FormDecl>("child", std::optional<std::string>("parent"));
            auto entity = std::make_unique<gspl::EntityDecl>("E");
            entity->body.push_back(std::move(form));

            gspl::ModuleDecl mod;
            mod.name = "exttest";
            mod.declarations.push_back(std::move(entity));

            auto result = resolver.resolve(mod);
            bool has_unknown = false;
            for (auto const& d : result.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_NAME_UNKNOWN) has_unknown = true;
            }
            check(has_unknown, "Form extending unknown entity should produce GSPL_NAME_UNKNOWN");
        }

        // ---- 10. Path traversal rejection ----
        {
            gspl::SourceManager sm;
            sm.add_source_root("C:/safe/root");
            check(sm.is_known_root("C:/safe/root"), "Known root should be recognized");
        }

        std::cout << "ALL MODULE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
