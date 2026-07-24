#include "gspl/studio/bracket_matcher.hpp"
#include <stack>
#include <cctype>

namespace gspl::studio {

bool BracketMatcher::is_bracket(char c) {
    return is_open_bracket(c) || is_close_bracket(c);
}

bool BracketMatcher::is_open_bracket(char c) {
    return c == '(' || c == '[' || c == '{';
}

bool BracketMatcher::is_close_bracket(char c) {
    return c == ')' || c == ']' || c == '}';
}

char BracketMatcher::matching_bracket(char c) {
    switch (c) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        default: return 0;
    }
}

std::optional<size_t> BracketMatcher::find_match(std::string_view text, size_t pos) {
    if (pos >= text.size()) return std::nullopt;
    char c = text[pos];
    if (!is_bracket(c)) return std::nullopt;

    char target = matching_bracket(c);
    int direction = is_open_bracket(c) ? 1 : -1;
    int depth = 0;

    for (size_t i = pos + direction; i < text.size() && i != static_cast<size_t>(-1); i += static_cast<size_t>(direction)) {
        if (text[i] == '"' || text[i] == '\'') {
            // Skip string literals
            char quote = text[i];
            ++i;
            while (i < text.size() && text[i] != quote) {
                if (text[i] == '\\' && i + 1 < text.size()) ++i;
                ++i;
            }
            if (i >= text.size()) break;
            continue;
        }
        if (text[i] == '/' && i + 1 < text.size()) {
            if (text[i + 1] == '/') {
                // Skip line comment
                while (i < text.size() && text[i] != '\n') ++i;
                continue;
            }
            if (text[i + 1] == '*') {
                // Skip block comment
                i += 2;
                while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) ++i;
                if (i < text.size()) ++i;
                continue;
            }
        }
        if (text[i] == c) ++depth;
        if (text[i] == target) {
            if (depth == 0) return i;
            --depth;
        }
    }
    return std::nullopt;
}

std::vector<BracketPair> BracketMatcher::find_all_pairs(std::string_view text) {
    std::vector<BracketPair> pairs;
    std::stack<BracketPair> open_stack;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '"' || c == '\'') {
            char quote = c; ++i;
            while (i < text.size() && text[i] != quote) {
                if (text[i] == '\\' && i + 1 < text.size()) ++i;
                ++i;
            }
            continue;
        }
        if (c == '/' && i + 1 < text.size()) {
            if (text[i + 1] == '/') { while (i < text.size() && text[i] != '\n') ++i; continue; }
            if (text[i + 1] == '*') { i += 2; while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) ++i; continue; }
        }
        if (is_open_bracket(c)) {
            open_stack.push({i, 0, c, matching_bracket(c), false});
        } else if (is_close_bracket(c)) {
            if (!open_stack.empty() && open_stack.top().close_char == c) {
                auto pair = open_stack.top(); open_stack.pop();
                pair.close_pos = i;
                pair.matched = true;
                pairs.push_back(pair);
            }
        }
    }
    // Add unmatched open brackets
    while (!open_stack.empty()) {
        pairs.push_back(open_stack.top());
        open_stack.pop();
    }
    return pairs;
}

std::optional<BracketPair> BracketMatcher::containing_pair(std::string_view text, size_t cursor_pos) {
    auto pairs = find_all_pairs(text);
    for (auto const& p : pairs) {
        if (p.matched && cursor_pos >= p.open_pos && cursor_pos <= p.close_pos)
            return p;
    }
    return std::nullopt;
}

size_t BracketMatcher::detect_indent(std::string_view line) {
    size_t level = 0;
    for (char c : line) {
        if (c == ' ') continue;
        if (c == '\t') { ++level; continue; }
        break;
    }
    return level;
}

IndentResult BracketMatcher::auto_indent(std::string_view previous_line, std::string_view current_line, size_t tab_size) {
    IndentResult result;
    
    // Detect current indent
    size_t base_indent = detect_indent(previous_line);
    
    // Check if previous line ends with an opening bracket
    std::string trimmed;
    for (auto it = previous_line.rbegin(); it != previous_line.rend(); ++it) {
        char c = *it;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        trimmed = c;
        break;
    }
    
    if (trimmed == "{" || trimmed == "(" || trimmed == "[") {
        result.indent_level = base_indent + 1;
        result.increase_next = true;
    } else {
        // Check if current line starts with a closing bracket
        bool starts_with_close = false;
        for (char c : current_line) {
            if (c == ' ' || c == '\t') continue;
            if (c == '}' || c == ')' || c == ']') starts_with_close = true;
            break;
        }
        if (starts_with_close && base_indent > 0)
            result.indent_level = base_indent - 1;
        else
            result.indent_level = base_indent;
    }
    
    result.line = indent_string(result.indent_level, tab_size);
    result.line += current_line;
    return result;
}

std::string BracketMatcher::indent_string(size_t level, size_t tab_size) {
    return std::string(level * tab_size, ' ');
}

} // namespace gspl::studio
