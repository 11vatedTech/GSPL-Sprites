#include "gspl/ls/ls_server.hpp"
#include "gspl/ls/diagnostic.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include <cassert>
#include <cstdio>
#include <sstream>

void test_ls_initialize() {
    gspl::ls::LsServer server;
    server.initialize("/test/workspace");
    gspl::ls::LsRequest req;
    req.method = "initialize";
    req.params = "{}";
    auto resp = server.handle_request_sync(req);
    assert(!resp.error);
    assert(resp.result.find("capabilities") != std::string::npos);
    server.shutdown();
}

void test_ls_completions() {
    gspl::ls::CompletionItem item;
    item.label = "test";
    item.kind = gspl::ls::CompletionItemKind::Keyword;
    item.insert_text = "test";
    
    gspl::ls::CompletionList list;
    list.items.push_back(item);
    assert(!list.items.empty());
    
    auto json = gspl::ls::completion_list_to_json(list);
    assert(json.find("test") != std::string::npos);
}

void test_diagnostic_roundtrip() {
    gspl::ls::Diagnostic diag;
    diag.message = "test error";
    diag.severity = gspl::ls::DiagnosticSeverity::Error;
    diag.range.start.line = 1;
    diag.range.start.column = 5;
    diag.range.end.line = 1;
    diag.range.end.column = 10;
    
    auto json = gspl::ls::diagnostic_to_json(diag);
    assert(json.find("test error") != std::string::npos);
    assert(json.find("severity") != std::string::npos);
}

void test_symbol_info_json() {
    gspl::ls::SymbolInfo sym;
    sym.name = "TestEntity";
    sym.kind = "entity";
    sym.range.start.line = 0;
    sym.range.start.column = 0;
    sym.range.end.line = 10;
    sym.range.end.column = 0;
    sym.selection_range = sym.range;
    
    auto json = gspl::ls::symbol_info_to_json(sym);
    assert(json.find("TestEntity") != std::string::npos);
}

void test_location_json() {
    gspl::ls::Location loc;
    loc.uri = "file:///test.gspl";
    loc.range.start.line = 5;
    loc.range.start.column = 10;
    loc.range.end.line = 5;
    loc.range.end.column = 20;
    
    auto json = gspl::ls::location_to_json(loc);
    assert(json.find("test.gspl") != std::string::npos);
}

int main() {
    test_ls_initialize();
    test_ls_completions();
    test_diagnostic_roundtrip();
    test_symbol_info_json();
    test_location_json();
    std::printf("All LS tests passed.\n");
    return 0;
}
