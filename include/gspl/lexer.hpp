#pragma once
#include "gspl/token.hpp"
#include "gspl/source.hpp"
#include "gspl/diagnostics.hpp"
#include <vector>

namespace gspl {

struct LexerConfig {
    std::uint64_t max_token_length{1024};
    std::uint64_t max_string_length{4096};
    std::uint64_t max_comment_length{65536};
};

class Lexer {
public:
    explicit Lexer(SourceBuffer const& source, LexerConfig config = {});
    Token next();
    std::vector<Token> tokenize();
    DiagnosticResult const& diagnostics() const { return diags_; }
    bool has_error() const { return !diags_.ok(); }
private:
    SourceBuffer const& source_;
    LexerConfig config_;
    DiagnosticResult diags_;
    std::uint64_t offset_{};
    std::uint32_t line_{1};
    std::uint32_t column_{1};
    char peek(std::uint64_t ahead = 0) const;
    char advance();
    void skip_whitespace();
    Token lex_identifier_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_comment_line();
    Token lex_comment_block();
    Token lex_operator_or_delimiter();
    SourceLocation current_location() const;
    SourceSpan make_span(std::uint64_t start_off, std::uint64_t end_off,
                         std::uint32_t start_line, std::uint32_t start_col) const;
    void add_error(DiagnosticCode code, std::string msg);
};

} // namespace gspl
