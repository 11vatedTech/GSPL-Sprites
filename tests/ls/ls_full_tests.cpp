#include "gspl/ls/ls_server.hpp"
#include "gspl/ls/diagnostic.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include "gspl/ls/document_symbols.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    ++tests_run; \
    try { \
        name(); \
        std::printf("  PASS  %s\n", #name); \
    } catch (const std::exception& e) { \
        std::printf("  FAIL  %s: %s\n", #name, e.what()); \
        ++tests_failed; \
    } catch (...) { \
        std::printf("  FAIL  %s: unknown exception\n", #name); \
        ++tests_failed; \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("assertion failed: " #cond); \
    } \
} while(0)

// --- Completion JSON serialization ---

static void test_completion_item_to_json() {
    gspl::ls::CompletionItem item;
    item.label = "test_label";
    item.kind = gspl::ls::CompletionItemKind::Keyword;
    item.detail = "a keyword";
    item.insert_text = "test_label";

    auto json = gspl::ls::completion_item_to_json(item);
    ASSERT(json.find("test_label") != std::string::npos);
    ASSERT(json.find("detail") != std::string::npos);
    ASSERT(json.find("insertText") != std::string::npos);
}

static void test_completion_list_to_json() {
    gspl::ls::CompletionList list;
    {
        gspl::ls::CompletionItem item;
        item.label = "alpha";
        item.kind = gspl::ls::CompletionItemKind::Keyword;
        list.items.push_back(item);
    }
    {
        gspl::ls::CompletionItem item;
        item.label = "beta";
        item.kind = gspl::ls::CompletionItemKind::Module;
        list.items.push_back(item);
    }
    list.is_incomplete = true;

    auto json = gspl::ls::completion_list_to_json(list);
    ASSERT(json.find("alpha") != std::string::npos);
    ASSERT(json.find("beta") != std::string::npos);
    ASSERT(json.find("isIncomplete") != std::string::npos);
    ASSERT(json.find("true") != std::string::npos);
}

// --- Completion filtering ---

static void test_completion_filter_exact_match() {
    gspl::ls::CompletionList list;
    {
        gspl::ls::CompletionItem item;
        item.label = "entity";
        list.items.push_back(item);
    }
    {
        gspl::ls::CompletionItem item;
        item.label = "entropy";
        list.items.push_back(item);
    }
    list.filter("entity");
    ASSERT(list.items.size() == 1);
    ASSERT(list.items[0].label == "entity");
}

static void test_completion_filter_case_insensitive() {
    gspl::ls::CompletionList list;
    {
        gspl::ls::CompletionItem item;
        item.label = "EntityDecl";
        list.items.push_back(item);
    }
    list.filter("entity");
    ASSERT(list.items.size() == 1);
}

static void test_completion_filter_empty_prefix() {
    gspl::ls::CompletionList list;
    {
        gspl::ls::CompletionItem item;
        item.label = "anything";
        list.items.push_back(item);
    }
    list.filter("");
    ASSERT(list.items.size() == 1);
}

static void test_completion_filter_no_match() {
    gspl::ls::CompletionList list;
    {
        gspl::ls::CompletionItem item;
        item.label = "foo";
        list.items.push_back(item);
    }
    list.filter("zzz");
    ASSERT(list.items.empty());
}

// --- Diagnostic JSON serialization ---

static void test_diagnostic_to_json() {
    gspl::ls::Diagnostic diag;
    diag.message = "test error message";
    diag.severity = gspl::ls::DiagnosticSeverity::Error;
    diag.code = "E001";
    diag.source = "gspl-ls";
    diag.range.start.line = 1;
    diag.range.start.column = 5;
    diag.range.end.line = 3;
    diag.range.end.column = 10;

    auto json = gspl::ls::diagnostic_to_json(diag);
    ASSERT(json.find("test error message") != std::string::npos);
    ASSERT(json.find("severity") != std::string::npos);
    ASSERT(json.find("source") != std::string::npos);
    ASSERT(json.find("gspl-ls") != std::string::npos);
}

static void test_diagnostic_from_compile_error() {
    gspl::Diagnostic compile_err;
    compile_err.code = gspl::DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN;
    compile_err.severity = gspl::DiagnosticSeverity::error;
    compile_err.message = "unexpected token";
    compile_err.span.start.line = 2;
    compile_err.span.start.column = 8;
    compile_err.span.end.line = 2;
    compile_err.span.end.column = 12;

    auto diag = gspl::ls::from_compile_error(compile_err, "test.gspl");
    ASSERT(diag.message == "unexpected token");
    ASSERT(diag.severity == gspl::ls::DiagnosticSeverity::Error);
    ASSERT(diag.range.start.line == 2);
    ASSERT(diag.range.start.column == 8);
    ASSERT(diag.range.end.column == 12);
    ASSERT(!diag.code.empty());
}

// --- Symbol info JSON ---

static void test_symbol_info_to_json() {
    gspl::ls::SymbolInfo sym;
    sym.name = "MyEntity";
    sym.kind = "entity";
    sym.range.start.line = 0;
    sym.range.start.column = 0;
    sym.range.end.line = 10;
    sym.range.end.column = 0;
    sym.selection_range = sym.range;

    auto json = gspl::ls::symbol_info_to_json(sym);
    ASSERT(json.find("MyEntity") != std::string::npos);
    ASSERT(json.find("entity") != std::string::npos);
}

static void test_symbol_info_with_children() {
    gspl::ls::SymbolInfo parent;
    parent.name = "Parent";
    parent.kind = "module";

    gspl::ls::SymbolInfo child;
    child.name = "Child";
    child.kind = "entity";
    parent.children.push_back(child);

    auto json = gspl::ls::symbol_info_to_json(parent);
    ASSERT(json.find("Parent") != std::string::npos);
    ASSERT(json.find("Child") != std::string::npos);
}

// --- Location JSON ---

static void test_location_to_json() {
    gspl::ls::Location loc;
    loc.uri = "file:///test.gspl";
    loc.range.start.line = 5;
    loc.range.start.column = 10;
    loc.range.end.line = 5;
    loc.range.end.column = 20;

    auto json = gspl::ls::location_to_json(loc);
    ASSERT(json.find("test.gspl") != std::string::npos);
    ASSERT(json.find("uri") != std::string::npos);
}

// --- LS Server basic operations ---

static void test_ls_initialize() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    gspl::ls::LsRequest req;
    req.method = "initialize";
    req.params = "{}";
    auto resp = server.handle_request_sync(req);
    ASSERT(!resp.error);
    ASSERT(resp.result.find("capabilities") != std::string::npos);

    server.shutdown();
}

static void test_ls_shutdown() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");
    server.shutdown();

    gspl::ls::LsRequest req;
    req.method = "shutdown";
    auto resp = server.handle_request_sync(req);
    ASSERT(!resp.error);
}

// --- Document symbols ---

static void test_document_symbols_empty() {
    gspl::ls::DocumentSymbolBuilder builder;
    auto symbols = builder.build("", "empty.gspl");
    ASSERT(symbols.empty());
}

static void test_document_symbols_invalid_source() {
    gspl::ls::DocumentSymbolBuilder builder;
    auto symbols = builder.build("@#$invalid", "bad.gspl");
    ASSERT(symbols.empty());
}

static void test_document_symbols_build_single_not_found() {
    gspl::ls::DocumentSymbolBuilder builder;
    auto sym = builder.build_single(
        "module foo {}", "test.gspl", "nonexistent");
    ASSERT(sym.name.empty());
}

// --- Completion integration ---

static void test_ls_keyword_completions() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("test.gspl", "entity Foo {}");
    auto items = server.get_completions("test.gspl", 0, 0);

    bool found_entity = false;
    for (const auto& item : items) {
        if (item.label == "entity") found_entity = true;
    }
    ASSERT(found_entity);

    server.shutdown();
}

// --- Diagnostic integration ---

static void test_ls_diagnostics_valid_source() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("valid.gspl", "entity Foo {}");
    auto diags = server.get_diagnostics("valid.gspl");

    ASSERT(!diags.empty());
    ASSERT(!diags[0].code.empty());

    server.shutdown();
}

static void test_ls_diagnostics_invalid_source() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("bad.gspl", "@@@invalid@@@ syntax error");
    auto diags = server.get_diagnostics("bad.gspl");

    ASSERT(!diags.empty());
    ASSERT(!diags[0].message.empty());

    server.shutdown();
}

// --- Malformed source recovery ---

static void test_ls_unterminated_string() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("bad_string.gspl", "let x = \"unterminated");
    auto diags = server.get_diagnostics("bad_string.gspl");

    ASSERT(!diags.empty());
    ASSERT(!diags[0].message.empty());

    server.shutdown();
}

static void test_ls_invalid_utf8() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    std::string invalid_utf8 = "entity \xFF\xFE {}";
    server.open_document("bad_utf8.gspl", invalid_utf8);
    auto diags = server.get_diagnostics("bad_utf8.gspl");

    ASSERT(!diags.empty());

    server.shutdown();
}

// --- Navigation ---

static void test_ls_goto_definition_unknown() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("test.gspl", "entity Foo {}");
    auto locations = server.goto_definition("test.gspl", 0, 0);
    ASSERT(locations.empty());

    server.shutdown();
}

static void test_ls_find_references_empty() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("test.gspl", "entity Foo {}");
    auto refs = server.find_references("test.gspl", 0, 0);
    ASSERT(refs.empty());

    server.shutdown();
}

static void test_ls_hover_unknown() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("test.gspl", "entity Foo {}");
    auto hover = server.get_hover("test.gspl", 0, 0);
    ASSERT(hover.contents.empty());

    server.shutdown();
}

// --- Symbol info navigation ---

static void test_ls_document_symbols() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");

    server.open_document("doc.gspl", "entity Foo { gene G1 {} form F1 {} }");
    auto symbols = server.get_document_symbols("doc.gspl");

    ASSERT(!symbols.empty());
    ASSERT(symbols[0].name == "Foo");

    server.shutdown();
}

int main() {
    std::printf("=== GSPL LS Full Tests ===\n\n");

    // Completion serialization
    TEST(test_completion_item_to_json);
    TEST(test_completion_list_to_json);

    // Completion filtering
    TEST(test_completion_filter_exact_match);
    TEST(test_completion_filter_case_insensitive);
    TEST(test_completion_filter_empty_prefix);
    TEST(test_completion_filter_no_match);

    // Diagnostic serialization
    TEST(test_diagnostic_to_json);
    TEST(test_diagnostic_from_compile_error);

    // Symbol info JSON
    TEST(test_symbol_info_to_json);
    TEST(test_symbol_info_with_children);

    // Location JSON
    TEST(test_location_to_json);

    // LS Server operations
    TEST(test_ls_initialize);
    TEST(test_ls_shutdown);

    // Document symbols
    TEST(test_document_symbols_empty);
    TEST(test_document_symbols_invalid_source);

    // Completion integration
    TEST(test_ls_keyword_completions);

    // Diagnostics
    TEST(test_ls_diagnostics_valid_source);
    TEST(test_ls_diagnostics_invalid_source);

    // Malformed source recovery
    TEST(test_ls_unterminated_string);
    TEST(test_ls_invalid_utf8);

    // Navigation
    TEST(test_ls_goto_definition_unknown);
    TEST(test_ls_find_references_empty);
    TEST(test_ls_hover_unknown);

    // Document symbols integration
    TEST(test_ls_document_symbols);

    std::printf("\n=== Results: %d tests, %d failed ===\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
