#include "gspl/parser.hpp"
#include <ranges>

namespace gspl {

Parser::Parser(std::vector<Token> tokens, SourceManager const& sources, ParserConfig config)
    : tokens_(std::move(tokens)), sources_(sources), config_(config) {}

Token const& Parser::peek(std::size_t ahead) const {
    auto idx = pos_ + ahead;
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
}

Token const& Parser::advance() {
    if (pos_ < tokens_.size()) ++pos_;
    return previous();
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) { advance(); return true; }
    return false;
}

bool Parser::check(TokenKind kind) const {
    return peek().kind == kind;
}

Token const& Parser::previous() const {
    return pos_ > 0 ? tokens_[pos_ - 1] : tokens_[0];
}

void Parser::synchronize() {
    while (!check(TokenKind::end_of_file)) {
        switch (peek().kind) {
        case TokenKind::keyword_module:
        case TokenKind::keyword_entity:
        case TokenKind::keyword_gene:
        case TokenKind::keyword_form:
        case TokenKind::keyword_import:
        case TokenKind::keyword_morphology:
        case TokenKind::keyword_resource:
        case TokenKind::keyword_ability:
            return;
        default: advance();
        }
    }
}

Token const& Parser::expect(TokenKind kind, DiagnosticCode code, std::string msg) {
    if (check(kind)) return advance();
    diags_.add_error(code, std::move(msg), peek().span);
    return peek();
}

std::unique_ptr<IdentifierRef> Parser::parse_identifier() {
    auto tok = expect(TokenKind::identifier, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
                       "Expected identifier");
    return std::make_unique<IdentifierRef>(tok.text);
}

std::unique_ptr<TypeRef> Parser::parse_type_ref() {
    auto tok = expect(TokenKind::identifier, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
                       "Expected type name");
    auto ref = std::make_unique<TypeRef>(tok.text);
    if (match(TokenKind::less)) {
        do { ref->type_args.push_back(parse_type_ref()); }
        while (match(TokenKind::comma));
        expect(TokenKind::greater, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN, "Expected '>'");
    }
    return ref;
}

std::unique_ptr<LiteralNode> Parser::parse_literal() {
    auto tok = peek();
    if (tok.kind == TokenKind::integer_literal || tok.kind == TokenKind::unsigned_literal ||
        tok.kind == TokenKind::fixed_literal || tok.kind == TokenKind::string_literal ||
        tok.kind == TokenKind::boolean_literal || tok.kind == TokenKind::duration_literal ||
        tok.kind == TokenKind::distance_literal || tok.kind == TokenKind::angle_literal ||
        tok.kind == TokenKind::percentage_literal || tok.kind == TokenKind::color_literal) {
        return std::make_unique<LiteralNode>(tok.kind, tok.text);
    }
    return nullptr;
}

std::unique_ptr<AstNode> Parser::parse_attribute() {
    auto key = expect(TokenKind::identifier, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
                       "Expected attribute key");
    expect(TokenKind::colon, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN, "Expected ':'");
    auto val = parse_literal();
    if (!val) {
        if (check(TokenKind::identifier)) val = std::make_unique<LiteralNode>(TokenKind::identifier, peek().text);
        else val = std::make_unique<LiteralNode>(TokenKind::boolean_literal, "true");
    }
    advance();
    return std::make_unique<AttributeNode>(key.text, std::move(val));
}

std::unique_ptr<ImportDecl> Parser::parse_import() {
    advance(); // consume 'import'
    std::string path;
    while (check(TokenKind::identifier) || check(TokenKind::dot)) {
        if (check(TokenKind::dot)) { path += '.'; advance(); }
        else { path += peek().text; advance(); }
    }
    auto decl = std::make_unique<ImportDecl>(path);
    if (match(TokenKind::keyword_namespace)) {
        decl->alias = parse_identifier()->name;
    }
    expect(TokenKind::semicolon, DiagnosticCode::GSPL_PARSE_MISSING_SEMICOLON,
           "Expected ';' after import");
    return decl;
}

std::unique_ptr<EntityDecl> Parser::parse_entity() {
    advance(); // consume 'entity'
    auto name = parse_identifier()->name;
    auto decl = std::make_unique<EntityDecl>(name);
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        auto child = parse_declaration();
        if (child) decl->body.push_back(std::move(child));
        else advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<GeneDecl> Parser::parse_gene() {
    advance(); // consume 'gene'
    auto name = parse_identifier()->name;
    std::vector<std::string> deps;
    if (match(TokenKind::colon)) {
        do { deps.push_back(parse_identifier()->name); }
        while (match(TokenKind::comma));
    }
    auto decl = std::make_unique<GeneDecl>(name, std::move(deps));
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        if (match(TokenKind::semicolon)) continue;
        auto attr = parse_attribute();
        if (attr) decl->body.push_back(std::move(attr));
        else advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<FormDecl> Parser::parse_form() {
    advance(); // consume 'form'
    auto name = parse_identifier()->name;
    std::optional<std::string> ext;
    if (match(TokenKind::colon)) {
        ext = parse_identifier()->name;
    }
    auto decl = std::make_unique<FormDecl>(name, ext);
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        auto child = parse_declaration();
        if (child) decl->body.push_back(std::move(child));
        else advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<TransformationDecl> Parser::parse_transformation() {
    advance();
    auto name = parse_identifier()->name;
    auto from = parse_identifier()->name;
    auto to = parse_identifier()->name;
    auto decl = std::make_unique<TransformationDecl>(name, from, to);
    if (!check(TokenKind::semicolon)) {
        expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
        while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) advance();
        expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    }
    return decl;
}

std::unique_ptr<MorphologyDecl> Parser::parse_morphology() {
    advance();
    auto decl = std::make_unique<MorphologyDecl>();
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        if (match(TokenKind::semicolon)) continue;
        advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<ResourceDecl> Parser::parse_resource() {
    advance();
    auto name = parse_identifier()->name;
    auto type_tok = expect(TokenKind::identifier, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
                            "Expected resource type");
    auto decl = std::make_unique<ResourceDecl>(name, type_tok.text);
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        if (match(TokenKind::semicolon)) continue;
        auto attr = parse_attribute();
        if (attr) decl->body.push_back(std::move(attr));
        else advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<AbilityDecl> Parser::parse_ability() {
    advance();
    auto name = parse_identifier()->name;
    auto decl = std::make_unique<AbilityDecl>(name);
    expect(TokenKind::lbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '{'");
    while (!check(TokenKind::rbrace) && !check(TokenKind::end_of_file)) {
        auto child = parse_declaration();
        if (child) decl->body.push_back(std::move(child));
        else advance();
    }
    expect(TokenKind::rbrace, DiagnosticCode::GSPL_PARSE_UNBALANCED_BRACE, "Expected '}'");
    return decl;
}

std::unique_ptr<RightsDecl> Parser::parse_rights() {
    advance();
    auto classification = parse_identifier()->name;
    auto code = parse_identifier()->name;
    match(TokenKind::semicolon);
    return std::make_unique<RightsDecl>(classification + "/" + code);
}

std::unique_ptr<AstNode> Parser::parse_declaration() {
    if (check(TokenKind::end_of_file)) return nullptr;
    if (nesting_depth_ > config_.max_nesting_depth) {
        diags_.add_error(DiagnosticCode::GSPL_PARSE_NESTING_EXCEEDED,
                         "Exceeded maximum nesting depth", peek().span);
        return nullptr;
    }
    ++nesting_depth_;
    std::unique_ptr<AstNode> result;
    switch (peek().kind) {
    case TokenKind::keyword_import: result = parse_import(); break;
    case TokenKind::keyword_entity: result = parse_entity(); break;
    case TokenKind::keyword_gene: result = parse_gene(); break;
    case TokenKind::keyword_form: result = parse_form(); break;
    case TokenKind::keyword_transformation: result = parse_transformation(); break;
    case TokenKind::keyword_morphology: result = parse_morphology(); break;
    case TokenKind::keyword_resource: result = parse_resource(); break;
    case TokenKind::keyword_ability: result = parse_ability(); break;
    case TokenKind::keyword_rights: result = parse_rights(); break;
    default:
        diags_.add_error(DiagnosticCode::GSPL_PARSE_INVALID_DECLARATION,
                         "Unexpected token in declaration: " + std::string(token_kind_name(peek().kind)),
                         peek().span);
        advance();
        break;
    }
    --nesting_depth_;
    return result;
}

std::unique_ptr<ModuleDecl> Parser::parse_module() {
    auto mod = std::make_unique<ModuleDecl>();
    expect(TokenKind::keyword_module, DiagnosticCode::GSPL_PARSE_UNEXPECTED_TOKEN,
           "Expected 'module' at start of file");
    mod->name = parse_identifier()->name;
    expect(TokenKind::semicolon, DiagnosticCode::GSPL_PARSE_MISSING_SEMICOLON,
           "Expected ';' after module name");

    while (!check(TokenKind::end_of_file)) {
        if (check(TokenKind::keyword_import)) {
            auto imp = parse_import();
            if (imp) mod->imports.push_back(std::move(imp));
            continue;
        }
        auto decl = parse_declaration();
        if (decl) {
            mod->declarations.push_back(std::move(decl));
        } else if (!check(TokenKind::end_of_file)) {
            advance();
        }
    }
    return mod;
}

} // namespace gspl
