#pragma once
#include "gspl/source.hpp"
#include <cstdint>
#include <string>

namespace gspl {

enum class TokenKind : std::uint8_t {
    identifier, qualified_identifier,
    keyword_module, keyword_import, keyword_entity, keyword_gene, keyword_trait,
    keyword_form, keyword_transformation, keyword_morphology, keyword_part,
    keyword_joint, keyword_socket, keyword_material, keyword_palette,
    keyword_animation, keyword_behavior, keyword_ability, keyword_resource,
    keyword_collision, keyword_rights, keyword_provenance, keyword_namespace,
    keyword_extends, keyword_override, keyword_remove, keyword_if, keyword_else,
    keyword_true, keyword_false, keyword_let, keyword_const, keyword_fn,
    integer_literal, unsigned_literal, fixed_literal, string_literal,
    duration_literal, distance_literal, angle_literal, percentage_literal,
    color_literal, boolean_literal,
    lbrace, rbrace, lparen, rparen, lbracket, rbracket,
    semicolon, colon, comma, dot, arrow, fat_arrow,
    plus, minus, star, slash, percent, equal, equal_equal,
    bang, bang_equal, less, less_equal, greater, greater_equal,
    ampersand_ampersand, pipe_pipe, caret,
    hash, at, tilde, question,
    comment_line, comment_block,
    error_token, end_of_file
};

struct Token {
    TokenKind kind{TokenKind::error_token};
    SourceSpan span;
    std::string text;
    std::string error_message;
    bool is_error() const { return kind == TokenKind::error_token; }
    Token() = default;
    Token(TokenKind k, SourceSpan s, std::string t)
        : kind(k), span(s), text(std::move(t)) {}
    Token(TokenKind k, SourceSpan s, std::string t, std::string em)
        : kind(k), span(s), text(std::move(t)), error_message(std::move(em)) {}
};

inline const char* token_kind_name(TokenKind k) {
    switch (k) {
    case TokenKind::identifier: return "identifier";
    case TokenKind::keyword_module: return "module";
    case TokenKind::keyword_entity: return "entity";
    case TokenKind::keyword_form: return "form";
    case TokenKind::keyword_transformation: return "transformation";
    case TokenKind::keyword_gene: return "gene";
    case TokenKind::keyword_import: return "import";
    case TokenKind::keyword_extends: return "extends";
    case TokenKind::keyword_override: return "override";
    case TokenKind::lbrace: return "{";
    case TokenKind::rbrace: return "}";
    case TokenKind::semicolon: return ";";
    case TokenKind::colon: return ":";
    case TokenKind::comma: return ",";
    case TokenKind::equal: return "=";
    case TokenKind::integer_literal: return "integer";
    case TokenKind::string_literal: return "string";
    case TokenKind::end_of_file: return "end of file";
    default: return "unknown";
    }
}

} // namespace gspl
