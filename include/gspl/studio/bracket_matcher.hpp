#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace gspl::studio {

struct BracketPair {
    size_t open_pos = 0;
    size_t close_pos = 0;
    char open_char = 0;
    char close_char = 0;
    bool matched = false;
};

struct IndentResult {
    std::string line;
    size_t indent_level = 0;
    bool increase_next = false;
};

class BracketMatcher {
public:
    static bool is_bracket(char c);
    static bool is_open_bracket(char c);
    static bool is_close_bracket(char c);
    static char matching_bracket(char c);

    // Find matching bracket for position; returns position or std::nullopt
    static std::optional<size_t> find_match(std::string_view text, size_t pos);

    // Find all bracket pairs in text
    static std::vector<BracketPair> find_all_pairs(std::string_view text);

    // Detect if cursor is inside a bracket pair; returns the pair if so
    static std::optional<BracketPair> containing_pair(std::string_view text, size_t cursor_pos);

    // Detect current indent level at a given line index
    static size_t detect_indent(std::string_view line);

    // Compute auto-indent for a new line after the given line
    static IndentResult auto_indent(std::string_view previous_line, std::string_view current_line = "",
                                     size_t tab_size = 4);

    // Compute the suggested indent for a line
    static std::string indent_string(size_t level, size_t tab_size = 4);
};

} // namespace gspl::studio
