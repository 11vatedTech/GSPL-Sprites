#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace gspl {

struct Utf8Result {
    bool valid{true};
    std::size_t offset{};
    std::string message;
};

Utf8Result validate_utf8(std::string_view input, std::size_t start_offset = 0);

bool is_utf8_continuation_byte(char c);

std::size_t utf8_sequence_length(char lead);

std::uint32_t utf8_codepoint(char const* bytes, std::size_t length);

bool has_bom(std::string const& input);

std::string strip_bom(std::string input);

std::size_t utf8_column_advance(std::string const& text, std::size_t offset);

enum class UnicodeCategory {
    ASCII,
    LETTER,
    DIGIT,
    UNDERSCORE,
    OPERATOR,
    WHITESPACE,
    CONTINUATION,
    INVALID
};

UnicodeCategory classify_utf8_byte(char c);

constexpr char BOM_UTF8_LE[] = "\xEF\xBB\xBF";

} // namespace gspl
