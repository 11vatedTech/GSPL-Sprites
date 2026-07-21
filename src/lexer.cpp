#include "gspl/lexer.hpp"
#include <cctype>
#include <cstdlib>

namespace gspl {

Lexer::Lexer(SourceBuffer const& source, LexerConfig config)
    : source_(source), config_(config) {}

char Lexer::peek(std::uint64_t ahead) const {
    auto idx = offset_ + ahead;
    return idx < source_.content().size() ? source_.content()[idx] : '\0';
}

char Lexer::advance() {
    if (offset_ >= source_.content().size()) return '\0';
    auto c = source_.content()[offset_++];
    if (c == '\n') { ++line_; column_ = 1; }
    else { ++column_; }
    return c;
}

SourceLocation Lexer::current_location() const {
    return {line_, column_};
}

SourceSpan Lexer::make_span(std::uint64_t start_off, std::uint64_t end_off,
                              std::uint32_t start_line, std::uint32_t start_col) const {
    auto end_loc = current_location();
    return {source_.id(), {start_line, start_col}, end_loc, start_off, end_off - start_off};
}

void Lexer::skip_whitespace() {
    while (offset_ < source_.content().size()) {
        auto c = source_.content()[offset_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') advance();
        else break;
    }
}

void Lexer::add_error(DiagnosticCode code, std::string msg) {
    diags_.add_error(code, std::move(msg), {source_.id(), current_location(), current_location(), offset_, 1});
}

Token Lexer::next() {
    skip_whitespace();
    if (offset_ >= source_.content().size()) {
        auto loc = current_location();
        return Token(TokenKind::end_of_file, {source_.id(), loc, loc, offset_, 0}, "");
    }
    auto c = peek();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return lex_identifier_or_keyword();
    if (std::isdigit(static_cast<unsigned char>(c))) return lex_number();
    if (c == '"') return lex_string();
    if (c == '/' && peek(1) == '/') return lex_comment_line();
    if (c == '/' && peek(1) == '*') return lex_comment_block();
    return lex_operator_or_delimiter();
}

Token Lexer::lex_identifier_or_keyword() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_' || peek() == '.') {
        advance();
    }
    auto text = std::string(source_.content().substr(start_off, offset_ - start_off));

    static constexpr struct { const char* name; TokenKind kind; } keywords[] = {
        {"module", TokenKind::keyword_module}, {"import", TokenKind::keyword_import},
        {"entity", TokenKind::keyword_entity}, {"gene", TokenKind::keyword_gene},
        {"trait", TokenKind::keyword_trait}, {"form", TokenKind::keyword_form},
        {"transformation", TokenKind::keyword_transformation},
        {"morphology", TokenKind::keyword_morphology},
        {"part", TokenKind::keyword_part}, {"joint", TokenKind::keyword_joint},
        {"socket", TokenKind::keyword_socket}, {"material", TokenKind::keyword_material},
        {"palette", TokenKind::keyword_palette}, {"animation", TokenKind::keyword_animation},
        {"behavior", TokenKind::keyword_behavior}, {"ability", TokenKind::keyword_ability},
        {"resource", TokenKind::keyword_resource}, {"collision", TokenKind::keyword_collision},
        {"rights", TokenKind::keyword_rights}, {"provenance", TokenKind::keyword_provenance},
        {"namespace", TokenKind::keyword_namespace}, {"extends", TokenKind::keyword_extends},
        {"override", TokenKind::keyword_override}, {"remove", TokenKind::keyword_remove},
        {"if", TokenKind::keyword_if}, {"else", TokenKind::keyword_else},
        {"true", TokenKind::keyword_true}, {"false", TokenKind::keyword_false},
        {"let", TokenKind::keyword_let}, {"const", TokenKind::keyword_const},
        {"fn", TokenKind::keyword_fn},
    };
    for (auto const& kw : keywords) {
        if (text == kw.name) {
            return Token(kw.kind, make_span(start_off, offset_, start_line, start_col), std::move(text));
        }
    }
    return Token(TokenKind::identifier, make_span(start_off, offset_, start_line, start_col), std::move(text));
}

Token Lexer::lex_number() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    bool is_hex = false;
    bool is_fixed = false;

    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        is_hex = true;
        advance(); advance();
        while (std::isxdigit(static_cast<unsigned char>(peek()))) advance();
    } else {
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }

    if (peek() == '.' && !is_hex) {
        is_fixed = true;
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }

    auto text = std::string(source_.content().substr(start_off, offset_ - start_off));
    auto kind = is_fixed ? TokenKind::fixed_literal
               : (is_hex ? TokenKind::unsigned_literal : TokenKind::integer_literal);
    return Token(kind, make_span(start_off, offset_, start_line, start_col), std::move(text));
}

Token Lexer::lex_string() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    advance();
    while (offset_ < source_.content().size() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (offset_ >= source_.content().size()) break;
        }
        if (offset_ - start_off > config_.max_string_length) {
            add_error(DiagnosticCode::GSPL_LEX_OVERSIZED_TOKEN, "String literal exceeds maximum length");
            break;
        }
        advance();
    }
    if (offset_ >= source_.content().size()) {
        add_error(DiagnosticCode::GSPL_LEX_UNTERMINATED_STRING, "Unterminated string literal");
    } else {
        advance();
    }
    auto text = std::string(source_.content().substr(start_off, offset_ - start_off));
    return Token(TokenKind::string_literal, make_span(start_off, offset_, start_line, start_col), std::move(text));
}

Token Lexer::lex_comment_line() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    while (offset_ < source_.content().size() && peek() != '\n') advance();
    auto text = std::string(source_.content().substr(start_off, offset_ - start_off));
    return Token(TokenKind::comment_line, make_span(start_off, offset_, start_line, start_col), std::move(text));
}

Token Lexer::lex_comment_block() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    advance(); advance();
    while (offset_ + 1 < source_.content().size()) {
        if (peek() == '*' && peek(1) == '/') { advance(); advance(); break; }
        if (offset_ - start_off > config_.max_comment_length) {
            add_error(DiagnosticCode::GSPL_LEX_OVERSIZED_TOKEN, "Block comment exceeds maximum length");
            break;
        }
        advance();
    }
    auto text = std::string(source_.content().substr(start_off, offset_ - start_off));
    return Token(TokenKind::comment_block, make_span(start_off, offset_, start_line, start_col), std::move(text));
}

Token Lexer::lex_operator_or_delimiter() {
    auto start_off = offset_;
    auto start_line = line_;
    auto start_col = column_;
    auto c = advance();
    auto sp = [&]() { return make_span(start_off, offset_, start_line, start_col); };

    switch (c) {
    case '{': return Token(TokenKind::lbrace, sp(), "{");
    case '}': return Token(TokenKind::rbrace, sp(), "}");
    case '(': return Token(TokenKind::lparen, sp(), "(");
    case ')': return Token(TokenKind::rparen, sp(), ")");
    case '[': return Token(TokenKind::lbracket, sp(), "[");
    case ']': return Token(TokenKind::rbracket, sp(), "]");
    case ';': return Token(TokenKind::semicolon, sp(), ";");
    case ':': return Token(TokenKind::colon, sp(), ":");
    case ',': return Token(TokenKind::comma, sp(), ",");
    case '.': return Token(TokenKind::dot, sp(), ".");
    case '+': return Token(TokenKind::plus, sp(), "+");
    case '-':
        if (peek() == '>') { advance(); return Token(TokenKind::arrow, sp(), "->"); }
        return Token(TokenKind::minus, sp(), "-");
    case '*': return Token(TokenKind::star, sp(), "*");
    case '/': return Token(TokenKind::slash, sp(), "/");
    case '%': return Token(TokenKind::percent, sp(), "%");
    case '=':
        if (peek() == '=') { advance(); return Token(TokenKind::equal_equal, sp(), "=="); }
        return Token(TokenKind::equal, sp(), "=");
    case '!':
        if (peek() == '=') { advance(); return Token(TokenKind::bang_equal, sp(), "!="); }
        return Token(TokenKind::bang, sp(), "!");
    case '<':
        if (peek() == '=') { advance(); return Token(TokenKind::less_equal, sp(), "<="); }
        return Token(TokenKind::less, sp(), "<");
    case '>':
        if (peek() == '=') { advance(); return Token(TokenKind::greater_equal, sp(), ">="); }
        return Token(TokenKind::greater, sp(), ">");
    case '&':
        if (peek() == '&') { advance(); return Token(TokenKind::ampersand_ampersand, sp(), "&&"); }
        return Token(TokenKind::error_token, sp(), "", "Unexpected '&'");
    case '|':
        if (peek() == '|') { advance(); return Token(TokenKind::pipe_pipe, sp(), "||"); }
        return Token(TokenKind::error_token, sp(), "", "Unexpected '|'");
    case '^': return Token(TokenKind::caret, sp(), "^");
    case '#': return Token(TokenKind::hash, sp(), "#");
    case '@': return Token(TokenKind::at, sp(), "@");
    case '~': return Token(TokenKind::tilde, sp(), "~");
    case '?': return Token(TokenKind::question, sp(), "?");
    default:
        add_error(DiagnosticCode::GSPL_LEX_INVALID_CHARACTER,
                  std::string("Unexpected character: '") + c + "'");
        return Token(TokenKind::error_token, sp(), std::string(1, c), "Unexpected character");
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    bool last_was_error = false;
    while (true) {
        auto tok = next();
        if (tok.kind == TokenKind::comment_line || tok.kind == TokenKind::comment_block) continue;
        bool is_err = tok.is_error();
        if (is_err && last_was_error) continue;
        tokens.push_back(tok);
        last_was_error = is_err;
        if (tok.kind == TokenKind::end_of_file) break;
    }
    return tokens;
}

} // namespace gspl
