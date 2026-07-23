#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace gspl::studio {

struct HighlightToken {
    std::uint64_t offset{0};
    std::uint64_t length{0};
    std::uint8_t color_index{0}; // 0=default, 1=keyword, 2=string, 3=number, 4=comment, 5=type, 6=function, 7=operator
};

struct HighlightLine {
    std::vector<HighlightToken> tokens;
};

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    
    std::vector<HighlightLine> highlight(std::string_view source) const;
    HighlightLine highlight_line(std::string_view line_source) const;

private:
    std::uint8_t color_for_keyword(std::string_view word) const;
};

} // namespace gspl::studio
