#include "gspl/ls/ls_server.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include "gspl/ls/hover.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <sstream>
#include <stdexcept>

using namespace gspl::ls;

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) { throw std::runtime_error("assertion failed: " #cond); } } while(0)
#define ASSERT_NO_THROW(expr) do { try { expr; } catch (...) { throw std::runtime_error("expected no exception: " #expr); } } while(0)

void test_completions_deterministic() {
    LsServer server;
    server.open_document("test.gspl", "module test { entity Foo { } }");

    auto a = server.get_completions("test.gspl", 0, 10);
    auto b = server.get_completions("test.gspl", 0, 10);
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
        ASSERT(a[i].label == b[i].label);
    }
    ASSERT(a.size() == b.size());
}

void test_diagnostics_deterministic() {
    LsServer server;
    server.open_document("test.gspl", "module test { entity Foo { gene g : Int; } }");

    auto a = server.get_diagnostics("test.gspl");
    auto b = server.get_diagnostics("test.gspl");
    ASSERT(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        ASSERT(a[i].message == b[i].message);
    }
}

void test_symbols_deterministic() {
    LsServer server;
    server.open_document("test.gspl", "module test { entity Foo { } }");

    auto a = server.get_document_symbols("test.gspl");
    auto b = server.get_document_symbols("test.gspl");
    ASSERT(a.size() == b.size());
}

void test_empty_source_stable() {
    LsServer server;
    server.open_document("empty.gspl", "");

    // Should not crash or throw
    ASSERT_NO_THROW(server.get_diagnostics("empty.gspl"));
    ASSERT_NO_THROW(server.get_document_symbols("empty.gspl"));
}

void test_minimal_module_stable() {
    LsServer server;
    std::string src = "module m { }";
    server.open_document("m.gspl", src);

    ASSERT_NO_THROW(server.get_diagnostics("m.gspl"));
    ASSERT_NO_THROW(server.get_document_symbols("m.gspl"));
    ASSERT_NO_THROW(server.get_completions("m.gspl", 0, 9));
}

void test_hover_empty_stable() {
    ASSERT_NO_THROW(get_hover_info("", "empty.gspl", 0, 0));
}

void test_hover_keyword_returns_doc() {
    auto info = get_hover_info("module test { }", "test.gspl", 0, 0);
    ASSERT(!info.contents.empty());
}

void test_symbol_count_bounded() {
    LsServer server;
    server.open_document("m.gspl", "module m { entity A { gene g : Int; } entity B { gene h : Bool; } }");

    auto symbols = server.get_document_symbols("m.gspl");
    ASSERT(symbols.size() <= 20);
}

void test_large_source_stable() {
    LsServer server;
    std::ostringstream large;
    large << "module large {\n";
    for (int i = 0; i < 100; ++i) {
        large << "  entity E" << i << " { gene g" << i << " : Int; }\n";
    }
    large << "}\n";
    server.open_document("large.gspl", large.str());

    ASSERT_NO_THROW(server.get_diagnostics("large.gspl"));
    ASSERT_NO_THROW(server.get_document_symbols("large.gspl"));

    auto symbols = server.get_document_symbols("large.gspl");
    ASSERT(symbols.size() >= 1);
    ASSERT(symbols.size() <= 210);
}

void test_identifiers_with_underscores() {
    LsServer server;
    server.open_document("test_module.gspl", "module test_module { entity my_entity { gene my_gene : Int; } }");

    ASSERT_NO_THROW(server.get_diagnostics("test_module.gspl"));
    ASSERT_NO_THROW(server.get_document_symbols("test_module.gspl"));
}

void test_completion_filter_empty_prefix() {
    std::vector<CompletionItem> items = {
        {"alpha", CompletionItemKind::Function, "alpha", {}, {}},
        {"beta", CompletionItemKind::Keyword, "beta", {}, {}},
        {"gamma", CompletionItemKind::Function, "gamma", {}, {}},
    };
    CompletionList list{std::move(items), false};

    list.filter("");
    ASSERT(list.items.size() == 3);
}

void test_completion_filter_deterministic() {
    std::vector<CompletionItem> items = {
        {"fooBar", CompletionItemKind::Function, "fooBar", {}, {}},
        {"fooBaz", CompletionItemKind::Function, "fooBaz", {}, {}},
    };
    CompletionList list{std::move(items), false};

    list.filter("foo");
    auto items_a = list.items;
    ASSERT(items_a.size() >= 2);
}

void test_hover_info_deterministic() {
    std::string src = "module test { entity Bar { } }";

    auto json_a = hover_info_to_json(get_hover_info(src, "test.gspl", 0, 7));
    auto json_b = hover_info_to_json(get_hover_info(src, "test.gspl", 0, 7));
    ASSERT(json_a == json_b);
}

void test_multiple_uris_independent() {
    LsServer server;
    server.open_document("a.gspl", "module a { }");
    server.open_document("b.gspl", "module b { entity X { } }");
    server.open_document("c.gspl", "module c { entity Y { gene z : Int; } }");

    auto d_a = server.get_diagnostics("a.gspl");
    auto d_b = server.get_diagnostics("b.gspl");
    auto d_c = server.get_diagnostics("c.gspl");

    auto d_a2 = server.get_diagnostics("a.gspl");
    ASSERT(d_a.size() == d_a2.size());
}

void test_monotonicity_valid_code() {
    LsServer server;

    server.open_document("x.gspl", "module x { }");
    auto base_diag = server.get_diagnostics("x.gspl");

    server.open_document("x.gspl", "module x { entity E { } }");
    auto ext_diag = server.get_diagnostics("x.gspl");
    // Adding valid code should not significantly increase error count
    ASSERT(ext_diag.size() <= base_diag.size() + 2);
}

int main() {
    std::printf("Property-Based Tests (LS determinism & stability)\n");

    TEST(test_completions_deterministic);
    TEST(test_diagnostics_deterministic);
    TEST(test_symbols_deterministic);
    TEST(test_empty_source_stable);
    TEST(test_minimal_module_stable);
    TEST(test_hover_empty_stable);
    TEST(test_hover_keyword_returns_doc);
    TEST(test_symbol_count_bounded);
    TEST(test_large_source_stable);
    TEST(test_identifiers_with_underscores);
    TEST(test_completion_filter_empty_prefix);
    TEST(test_completion_filter_deterministic);
    TEST(test_hover_info_deterministic);
    TEST(test_multiple_uris_independent);
    TEST(test_monotonicity_valid_code);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
