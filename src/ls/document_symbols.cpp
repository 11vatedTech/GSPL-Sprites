#include "gspl/ls/document_symbols.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"

namespace gspl::ls {

SymbolInfo DocumentSymbolBuilder::make_symbol(std::string_view name, std::string_view kind,
                                              uint32_t start_line, uint32_t start_col,
                                              uint32_t end_line, uint32_t end_col) {
    SymbolInfo sym;
    sym.name = name;
    sym.kind = kind;
    sym.range.start.line = start_line;
    sym.range.start.column = start_col;
    sym.range.end.line = end_line;
    sym.range.end.column = end_col;
    sym.selection_range = sym.range;
    return sym;
}

std::vector<SymbolInfo> DocumentSymbolBuilder::build(std::string_view source, std::string_view uri) {
    std::vector<SymbolInfo> symbols;
    if (source.empty()) return symbols;
    
    auto buf = gspl::SourceBuffer::from_string(std::string(uri), std::string(source));
    gspl::SourceManager sources;
    auto buf_id = sources.register_buffer(std::move(buf));

    gspl::LexerConfig lex_cfg;
    gspl::Lexer lexer(*sources.lookup(static_cast<gspl::SourceId>(buf_id)), lex_cfg);
    auto tokens = lexer.tokenize();

    gspl::Parser parser(tokens, sources);
    auto module = parser.parse_module();
    if (!module) return symbols;

    auto decl_name = [](gspl::AstNode const* n) -> std::string_view {
        using enum gspl::AstKind;
        switch (n->kind) {
        case entity:        return static_cast<gspl::EntityDecl const*>(n)->name;
        case gene:          return static_cast<gspl::GeneDecl const*>(n)->name;
        case form:          return static_cast<gspl::FormDecl const*>(n)->name;
        case transformation:return static_cast<gspl::TransformationDecl const*>(n)->name;
        case part:          return static_cast<gspl::PartDecl const*>(n)->name;
        case resource:      return static_cast<gspl::ResourceDecl const*>(n)->name;
        case ability:       return static_cast<gspl::AbilityDecl const*>(n)->name;
        case block:         return static_cast<gspl::GenericBlock const*>(n)->name;
        default:            return {};
        }
    };

    for (const auto& decl : module->declarations) {
        auto name = decl_name(decl.get());
        if (name.empty()) continue;
        symbols.push_back(make_symbol(name, "declaration",
            decl->span.start.line, decl->span.start.column,
            decl->span.end.line, decl->span.end.column));
    }
    
    return symbols;
}

SymbolInfo DocumentSymbolBuilder::build_single(std::string_view source, std::string_view uri, std::string_view name) {
    auto symbols = build(source, uri);
    for (const auto& sym : symbols) {
        if (sym.name == name) return sym;
    }
    return {};
}

} // namespace gspl::ls
