#include "gspl/ls/ls_server.hpp"
#include "gspl/ls/diagnostic.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"
#include "gspl/diagnostics.hpp"
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>

namespace gspl::ls {

struct TrackedDocument {
    std::string uri;
    std::string content;
    std::string language_id;
    std::uint64_t version{0};
};

struct SymbolIndexEntry {
    std::string name;
    std::string kind;
    std::string uri;
    Range range;
    Range selection_range;
};

struct LsServer::Impl {
    std::string workspace_root;
    bool initialized{false};
    std::unordered_map<std::string, TrackedDocument> documents;
    std::vector<SymbolIndexEntry> symbol_index;
    std::unordered_map<std::string, std::vector<std::size_t>> symbol_by_name;

    void build_symbol_index();
    std::vector<Diagnostic> diagnose_document(const TrackedDocument& doc) const;
    std::vector<CompletionItem> get_completions(const TrackedDocument& doc, std::uint32_t line, std::uint32_t column) const;
    std::vector<Location> goto_definition(const TrackedDocument&, std::uint32_t, std::uint32_t) const;
    std::vector<Reference> find_references(const TrackedDocument&, std::uint32_t, std::uint32_t) const;
    HoverInfo get_hover(const TrackedDocument&, std::uint32_t, std::uint32_t) const;
    std::vector<SymbolInfo> get_document_symbols(const TrackedDocument&) const;
};

LsServer::LsServer() : impl_(std::make_unique<Impl>()) {}
LsServer::~LsServer() = default;

void LsServer::initialize(std::string_view workspace_root) {
    impl_->workspace_root = workspace_root;
    impl_->initialized = true;
}

void LsServer::shutdown() {
    impl_->documents.clear();
    impl_->symbol_index.clear();
    impl_->symbol_by_name.clear();
    impl_->initialized = false;
}

void LsServer::handle_request(LsRequest req, SendResponse send) {
    LsResponse resp;
    resp.id = req.id;

    if (req.method == "initialize") {
        resp.result = R"({"capabilities":{"textDocumentSync":1,"completionProvider":{},"hoverProvider":true,"definitionProvider":true,"referencesProvider":true,"renameProvider":true,"documentSymbolProvider":true}})";
    } else if (req.method == "shutdown") {
        shutdown();
        resp.result = "{}";
    } else if (req.method == "textDocument/didOpen") {
        auto uri_start = req.params.find("\"uri\"");
        if (uri_start != std::string::npos) {
            uri_start = req.params.find('"', uri_start + 6);
            auto uri_end = req.params.find('"', uri_start + 1);
            if (uri_start != std::string::npos && uri_end != std::string::npos) {
                std::string uri = req.params.substr(uri_start + 1, uri_end - uri_start - 1);
                TrackedDocument doc;
                doc.uri = uri;
                doc.language_id = "gspl";
                auto text_start = req.params.find("\"text\"");
                if (text_start != std::string::npos) {
                    auto ts = req.params.find('"', text_start + 6);
                    auto te = req.params.rfind('"');
                    if (ts != std::string::npos && te != std::string::npos && te > ts) {
                        doc.content = req.params.substr(ts + 1, te - ts - 1);
                    }
                }
                impl_->documents[uri] = doc;
                impl_->build_symbol_index();
            }
        }
        resp.result = "{}";
    } else if (req.method == "textDocument/didChange") {
        if (!impl_->documents.empty()) {
            auto& doc = impl_->documents.begin()->second;
            auto text_start = req.params.find("\"text\"");
            if (text_start != std::string::npos) {
                auto ts = req.params.find('"', text_start + 6);
                auto te = req.params.rfind('"');
                if (ts != std::string::npos && te != std::string::npos && te > ts) {
                    doc.content = req.params.substr(ts + 1, te - ts - 1);
                }
            }
            impl_->build_symbol_index();
        }
        resp.result = "{}";
    } else if (req.method == "textDocument/didClose") {
        auto uri_start = req.params.find("\"uri\"");
        if (uri_start != std::string::npos) {
            uri_start = req.params.find('"', uri_start + 6);
            auto uri_end = req.params.find('"', uri_start + 1);
            if (uri_start != std::string::npos && uri_end != std::string::npos) {
                std::string uri = req.params.substr(uri_start + 1, uri_end - uri_start - 1);
                impl_->documents.erase(uri);
                impl_->build_symbol_index();
            }
        }
        resp.result = "{}";
    } else if (req.method == "textDocument/completion") {
        if (!impl_->documents.empty()) {
            auto completions = impl_->get_completions(impl_->documents.begin()->second, 0, 0);
            std::ostringstream oss;
            oss << "{\"isIncomplete\":false,\"items\":[";
            for (size_t i = 0; i < completions.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "{\"label\":\"" << completions[i].label
                    << "\",\"kind\":" << static_cast<int>(completions[i].kind) << "}";
            }
            oss << "]}";
            resp.result = oss.str();
        } else {
            resp.result = "{\"isIncomplete\":false,\"items\":[]}";
        }
    } else if (req.method == "textDocument/documentSymbol") {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < impl_->symbol_index.size(); ++i) {
            if (i > 0) oss << ",";
            auto& s = impl_->symbol_index[i];
            oss << "{\"name\":\"" << s.name << "\",\"kind\":12"
                << ",\"range\":{\"start\":{\"line\":" << s.range.start.line
                << ",\"character\":" << s.range.start.column
                << "},\"end\":{\"line\":" << s.range.end.line
                << ",\"character\":" << s.range.end.column << "}}"
                << ",\"selectionRange\":{\"start\":{\"line\":" << s.selection_range.start.line
                << ",\"character\":" << s.selection_range.start.column
                << "},\"end\":{\"line\":" << s.selection_range.end.line
                << ",\"character\":" << s.selection_range.end.column << "}}}";
        }
        oss << "]";
        resp.result = oss.str();
    } else if (req.method == "textDocument/hover") {
        resp.result = R"({"contents":{"kind":"markdown","value":"GSPL entity"}})";
    } else if (req.method == "textDocument/definition") {
        resp.result = "[]";
    } else if (req.method == "textDocument/references") {
        resp.result = "[]";
    } else if (req.method == "textDocument/rename") {
        resp.result = "{}";
    } else {
        resp.error = true;
        resp.error_message = "Unknown method: " + req.method;
    }

    send(resp);
}

LsResponse LsServer::handle_request_sync(LsRequest req) {
    LsResponse resp;
    resp.id = req.id;
    handle_request(req, [&](LsResponse r) { resp = r; });
    return resp;
}

void LsServer::open_document(std::string_view uri, std::string_view content) {
    TrackedDocument doc;
    doc.uri = uri;
    doc.content = content;
    doc.language_id = "gspl";
    impl_->documents[std::string(uri)] = doc;
    impl_->build_symbol_index();
}

std::vector<Diagnostic> LsServer::get_diagnostics(std::string_view uri) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->diagnose_document(it->second);
}

std::vector<CompletionItem> LsServer::get_completions(std::string_view uri, std::uint32_t line, std::uint32_t column) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->get_completions(it->second, line, column);
}

std::vector<Location> LsServer::goto_definition(std::string_view uri, std::uint32_t line, std::uint32_t column) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->goto_definition(it->second, line, column);
}

std::vector<Reference> LsServer::find_references(std::string_view uri, std::uint32_t line, std::uint32_t column) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->find_references(it->second, line, column);
}

HoverInfo LsServer::get_hover(std::string_view uri, std::uint32_t line, std::uint32_t column) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->get_hover(it->second, line, column);
}

std::vector<SymbolInfo> LsServer::get_document_symbols(std::string_view uri) const {
    auto it = impl_->documents.find(std::string(uri));
    if (it == impl_->documents.end()) return {};
    return impl_->get_document_symbols(it->second);
}

void LsServer::Impl::build_symbol_index() {
    symbol_index.clear();
    symbol_by_name.clear();

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

    for (const auto& [uri, doc] : documents) {
        auto buf = gspl::SourceBuffer::from_string(uri, doc.content);
        gspl::SourceManager sources;
        auto buf_id = sources.register_buffer(std::move(buf));

        gspl::LexerConfig lex_cfg;
        gspl::Lexer lexer(*sources.lookup(static_cast<gspl::SourceId>(buf_id)), lex_cfg);
        auto tokens = lexer.tokenize();

        gspl::Parser parser(tokens, sources);
        auto module = parser.parse_module();
        if (!module) continue;

        for (const auto& decl : module->declarations) {
            auto name = decl_name(decl.get());
            if (name.empty()) continue;
            SymbolIndexEntry entry;
            entry.name = std::string(name);
            entry.kind = "declaration";
            entry.uri = uri;
            entry.range.start.line = decl->span.start.line;
            entry.range.start.column = decl->span.start.column;
            entry.range.end.line = decl->span.end.line;
            entry.range.end.column = decl->span.end.column;
            entry.selection_range = entry.range;
            symbol_index.push_back(entry);
            symbol_by_name[entry.name].push_back(symbol_index.size() - 1);
        }
    }
}

std::vector<Diagnostic> LsServer::Impl::diagnose_document(const TrackedDocument& doc) const {
    std::vector<Diagnostic> diagnostics;

    auto buf = gspl::SourceBuffer::from_string(doc.uri, doc.content);
    gspl::SourceManager sources;
    auto buf_id = sources.register_buffer(std::move(buf));

    gspl::LexerConfig lex_cfg;
    gspl::Lexer lexer(*sources.lookup(static_cast<gspl::SourceId>(buf_id)), lex_cfg);
    auto tokens = lexer.tokenize();

    gspl::Parser parser(tokens, sources);
    parser.parse_module();

    for (const auto& gspl_diag : parser.diagnostics().diagnostics) {
        Diagnostic diag;
        diag.range.start.line = gspl_diag.span.start.line;
        diag.range.start.column = gspl_diag.span.start.column;
        diag.range.end.line = gspl_diag.span.end.line;
        diag.range.end.column = gspl_diag.span.end.column;
        diag.severity = static_cast<DiagnosticSeverity>(static_cast<int>(gspl_diag.severity));
        diag.message = gspl_diag.message;
        diag.code = std::to_string(static_cast<std::uint32_t>(gspl_diag.code));
        diagnostics.push_back(diag);
    }

    return diagnostics;
}

std::vector<CompletionItem> LsServer::Impl::get_completions(const TrackedDocument&, std::uint32_t, std::uint32_t) const {
    std::vector<CompletionItem> items;
    const char* keywords[] = {
        "module", "entity", "gene", "form", "morphology", "animation",
        "behavior", "combat", "import", "from", "as", "let", "fn",
        "if", "else", "match", "return", "true", "false", "null",
        "Int", "UInt", "Fixed", "Bool", "String", "Color", "Vector2",
        "Vector3", "Duration", "Distance", "Angle", "Percentage", "Ratio"
    };
    for (auto kw : keywords) {
        CompletionItem item;
        item.label = kw;
        item.kind = CompletionItemKind::Keyword;
        item.insert_text = kw;
        items.push_back(item);
    }
    for (const auto& [name, indices] : symbol_by_name) {
        CompletionItem item;
        item.label = name;
        item.kind = CompletionItemKind::Module;
        item.insert_text = name;
        items.push_back(item);
    }
    return items;
}

std::vector<Location> LsServer::Impl::goto_definition(const TrackedDocument&, std::uint32_t, std::uint32_t) const {
    return {};
}

std::vector<Reference> LsServer::Impl::find_references(const TrackedDocument&, std::uint32_t, std::uint32_t) const {
    return {};
}

HoverInfo LsServer::Impl::get_hover(const TrackedDocument&, std::uint32_t, std::uint32_t) const {
    return {};
}

std::vector<SymbolInfo> LsServer::Impl::get_document_symbols(const TrackedDocument& doc) const {
    std::vector<SymbolInfo> result;
    auto buf = gspl::SourceBuffer::from_string(doc.uri, doc.content);
    gspl::SourceManager sources;
    auto buf_id = sources.register_buffer(std::move(buf));

    gspl::LexerConfig lex_cfg;
    gspl::Lexer lexer(*sources.lookup(static_cast<gspl::SourceId>(buf_id)), lex_cfg);
    auto tokens = lexer.tokenize();

    gspl::Parser parser(tokens, sources);
    auto module = parser.parse_module();
    if (!module) return result;

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
        SymbolInfo sym;
        sym.name = std::string(name);
        sym.kind = "declaration";
        sym.range.start.line = decl->span.start.line;
        sym.range.start.column = decl->span.start.column;
        sym.range.end.line = decl->span.end.line;
        sym.range.end.column = decl->span.end.column;
        sym.selection_range = sym.range;
        result.push_back(sym);
    }

    return result;
}

} // namespace gspl::ls
