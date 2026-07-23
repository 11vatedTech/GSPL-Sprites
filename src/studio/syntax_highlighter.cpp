#include "gspl/studio/syntax_highlighter.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/token.hpp"
#include <unordered_set>

namespace gspl::studio {

SyntaxHighlighter::SyntaxHighlighter() = default;

std::vector<HighlightLine> SyntaxHighlighter::highlight(std::string_view source) const {
    std::vector<HighlightLine> lines;
    if (source.empty()) return lines;
    
    gspl::SourceFile src("", std::string(source));
    gspl::Lexer lexer(src);
    
    HighlightLine current_line;
    uint64_t line_start = 0;
    
    while (true) {
        auto token = lexer.next();
        if (token.type == gspl::TokenType::EndOfFile) break;
        
        // Track line changes
        while (token.location.line >= lines.size() + 1) {
            lines.push_back(current_line);
            current_line = HighlightLine{};
        }
        
        HighlightToken ht;
        ht.offset = token.location.column;
        ht.length = token.length;
        
        switch (token.type) {
            case gspl::TokenType::Keyword:
                ht.color_index = 1; break;
            case gspl::TokenType::StringLiteral:
                ht.color_index = 2; break;
            case gspl::TokenType::IntegerLiteral:
            case gspl::TokenType::FloatLiteral:
                ht.color_index = 3; break;
            case gspl::TokenType::Comment:
                ht.color_index = 4; break;
            case gspl::TokenType::Identifier:
                ht.color_index = color_for_keyword(token.text); break;
            default:
                ht.color_index = 7; break; // operators/punctuation
        }
        
        current_line.tokens.push_back(ht);
    }
    
    if (!current_line.tokens.empty() || lines.empty()) {
        lines.push_back(current_line);
    }
    
    return lines;
}

HighlightLine SyntaxHighlighter::highlight_line(std::string_view line_source) const {
    std::string full_src(line_source);
    gspl::SourceFile src("", full_src);
    gspl::Lexer lexer(src);
    
    HighlightLine line;
    while (true) {
        auto token = lexer.next();
        if (token.type == gspl::TokenType::EndOfFile) break;
        
        HighlightToken ht;
        ht.offset = token.location.column;
        ht.length = token.length;
        
        switch (token.type) {
            case gspl::TokenType::Keyword:
                ht.color_index = 1; break;
            case gspl::TokenType::StringLiteral:
                ht.color_index = 2; break;
            case gspl::TokenType::IntegerLiteral:
            case gspl::TokenType::FloatLiteral:
                ht.color_index = 3; break;
            case gspl::TokenType::Comment:
                ht.color_index = 4; break;
            case gspl::TokenType::Identifier:
                ht.color_index = color_for_keyword(token.text); break;
            default:
                ht.color_index = 7; break;
        }
        
        line.tokens.push_back(ht);
    }
    
    return line;
}

uint8_t SyntaxHighlighter::color_for_keyword(std::string_view word) const {
    static const std::unordered_set<std::string> types = {
        "Int", "UInt", "Fixed", "Bool", "String", "Color", "Vector2", "Vector3",
        "Duration", "Distance", "Angle", "Percentage", "Ratio"
    };
    if (types.count(std::string(word))) return 5; // type
    return 0; // default
}

} // namespace gspl::studio
