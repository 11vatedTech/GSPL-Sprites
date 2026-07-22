#pragma once
#include "gspl/ast.hpp"
#include "gspl/lexer.hpp"
#include "gspl/diagnostics.hpp"
#include <memory>

namespace gspl {

struct ParserConfig {
    std::uint64_t max_nesting_depth{32};
    std::uint64_t max_declaration_count{65536};
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, SourceManager const& sources, ParserConfig config = {});
    std::unique_ptr<ModuleDecl> parse_module();
    DiagnosticResult const& diagnostics() const { return diags_; }
    bool has_error() const { return !diags_.ok(); }
private:
    std::vector<Token> tokens_;
    SourceManager const& sources_;
    ParserConfig config_;
    DiagnosticResult diags_;
    std::size_t pos_{};
    std::uint64_t nesting_depth_{};
    Token const& peek(std::size_t ahead = 0) const;
    Token const& advance();
    bool match(TokenKind kind);
    bool check(TokenKind kind) const;
    Token const& previous() const;
    void synchronize();
    Token const& expect(TokenKind kind, DiagnosticCode code, std::string msg);
    std::unique_ptr<IdentifierRef> parse_identifier();
    std::unique_ptr<TypeRef> parse_type_ref();
    std::unique_ptr<LiteralNode> parse_literal();
    std::unique_ptr<AstNode> parse_attribute();
    std::unique_ptr<ImportDecl> parse_import();
    std::unique_ptr<EntityDecl> parse_entity();
    std::unique_ptr<GeneDecl> parse_gene();
    std::unique_ptr<FormDecl> parse_form();
    std::unique_ptr<TransformationDecl> parse_transformation();
    std::unique_ptr<MorphologyDecl> parse_morphology();
    std::unique_ptr<ResourceDecl> parse_resource();
    std::unique_ptr<AbilityDecl> parse_ability();
    std::unique_ptr<RightsDecl> parse_rights();
    std::unique_ptr<GenericBlock> parse_generic_block(std::string const& block_type);
    std::unique_ptr<AstNode> parse_declaration();
    std::unique_ptr<BlockNode> parse_block();
};

} // namespace gspl
