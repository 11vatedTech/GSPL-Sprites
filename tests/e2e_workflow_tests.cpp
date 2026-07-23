#include "gspl/ls/ls_server.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include "gspl/ls/diagnostic.hpp"
#include "gspl/ls/hover.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <sstream>
#include <chrono>
#include <stdexcept>
#include <filesystem>

using namespace gspl::ls;

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) { throw std::runtime_error("assertion failed: " #cond); } } while(0)
#define ASSERT_NO_THROW(expr) do { try { expr; } catch (...) { throw std::runtime_error("expected no exception: " #expr); } } while(0)

void test_e2e_lex_parse_diagnose_symbols() {
    const std::string source = R"(
module mygame {
    entity Hero {
        gene strength : Int;
        gene hp : Int;
    }
    entity Monster {
        gene damage : Int;
    }
}
)";

    // Lex
    auto buf = gspl::SourceBuffer::from_string("test.gspl", source);
    gspl::SourceManager mgr;
    auto id = mgr.register_buffer(std::move(buf));
    auto* sb = mgr.lookup(static_cast<gspl::SourceId>(id));
    ASSERT(sb != nullptr);

    gspl::LexerConfig lc;
    gspl::Lexer lexer(*sb, lc);
    auto tokens = lexer.tokenize();
    ASSERT(!tokens.empty());

    // Parse
    gspl::Parser parser(tokens, mgr);
    auto module = parser.parse_module();
    ASSERT(module != nullptr);
    ASSERT(module->declarations.size() >= 2);

    // LS: open and diagnose
    LsServer server;
    server.open_document("test.gspl", source);
    auto diagnostics = server.get_diagnostics("test.gspl");

    // Symbols
    auto symbols = server.get_document_symbols("test.gspl");
    ASSERT(symbols.size() >= 2);

    // JSON serialization
    ASSERT(symbol_info_to_json(symbols[0]).find("kind") != std::string::npos);
}

void test_e2e_invalid_source_diagnostics() {
    const std::string invalid_source = "module broken { this is not valid }";

    LsServer server;
    server.open_document("broken.gspl", invalid_source);
    ASSERT_NO_THROW(server.get_diagnostics("broken.gspl"));
    ASSERT_NO_THROW(server.get_document_symbols("broken.gspl"));
    ASSERT_NO_THROW(server.get_completions("broken.gspl", 0, 10));
}

void test_e2e_edit_source_re_diagnose() {
    LsServer server;

    server.open_document("m.gspl", "module m { }");
    auto sym_before = server.get_document_symbols("m.gspl");

    server.open_document("m.gspl", "module m { entity E { } }");
    auto sym_after = server.get_document_symbols("m.gspl");

    ASSERT(sym_after.size() >= sym_before.size());
}

void test_e2e_hover_on_keyword() {
    const std::string src = "module test { }";
    auto info = get_hover_info(src, "test.gspl", 0, 0);
    ASSERT(!info.contents.empty());
}

void test_e2e_hover_out_of_range() {
    const std::string src = "module test { }";
    ASSERT_NO_THROW(get_hover_info(src, "test.gspl", 99, 99));
}

void test_e2e_symbol_info_json_roundtrip() {
    SymbolInfo sym;
    sym.name = "TestEntity";
    sym.kind = "entity";
    sym.range = {{1, 2}, {3, 4}};
    sym.selection_range = {{1, 2}, {1, 10}};

    auto json = symbol_info_to_json(sym);
    ASSERT(!json.empty());
    ASSERT(json.find("TestEntity") != std::string::npos);
    ASSERT(json.find("\"kind\"") != std::string::npos);
    ASSERT(json.find("\"range\"") != std::string::npos);
}

void test_e2e_multiple_files_independent() {
    LsServer server;
    server.open_document("a.gspl", "module a { }");
    server.open_document("b.gspl", "module b { entity X { gene g : Int; } }");

    auto s1 = server.get_document_symbols("a.gspl");
    auto s2 = server.get_document_symbols("b.gspl");

    // The symbols should differ
    if (s1.size() == s2.size() && s1.size() == 1) {
        ASSERT(s1[0].name != s2[0].name);
    }
}

void test_e2e_large_module_stress() {
    std::ostringstream oss;
    oss << "module big {\n";
    for (int i = 0; i < 50; ++i) {
        oss << "  entity E" << i << " { gene g" << i << " : Int; }\n";
    }
    oss << "}\n";
    std::string source = oss.str();

    LsServer server;
    server.open_document("big.gspl", source);

    auto start = std::chrono::steady_clock::now();
    ASSERT_NO_THROW(server.get_diagnostics("big.gspl"));
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    ASSERT(ms < 5000);

    auto symbols = server.get_document_symbols("big.gspl");
    ASSERT(symbols.size() >= 1);
}

int main() {
    std::printf("E2E Workflow Tests\n");

    TEST(test_e2e_lex_parse_diagnose_symbols);
    TEST(test_e2e_invalid_source_diagnostics);
    TEST(test_e2e_edit_source_re_diagnose);
    TEST(test_e2e_hover_on_keyword);
    TEST(test_e2e_hover_out_of_range);
    TEST(test_e2e_symbol_info_json_roundtrip);
    TEST(test_e2e_multiple_files_independent);
    TEST(test_e2e_large_module_stress);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
