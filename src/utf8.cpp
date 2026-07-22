#include "gspl/utf8.hpp"
#include <cctype>
#include <cstring>
#include <string>

namespace gspl {

bool is_utf8_continuation_byte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

std::size_t utf8_sequence_length(char lead) {
    auto uc = static_cast<unsigned char>(lead);
    if (uc < 0x80) return 1;
    if ((uc & 0xE0) == 0xC0) return 2;
    if ((uc & 0xF0) == 0xE0) return 3;
    if ((uc & 0xF8) == 0xF0) return 4;
    return 0;
}

std::uint32_t utf8_codepoint(char const* bytes, std::size_t length) {
    if (length == 0) return 0;
    auto uc = static_cast<unsigned char>(bytes[0]);
    if (length == 1) return uc;
    if (length == 2) return (static_cast<std::uint32_t>(uc & 0x1F) << 6)
                          | static_cast<std::uint32_t>(bytes[1] & 0x3F);
    if (length == 3) return (static_cast<std::uint32_t>(uc & 0x0F) << 12)
                          | (static_cast<std::uint32_t>(bytes[1] & 0x3F) << 6)
                          | static_cast<std::uint32_t>(bytes[2] & 0x3F);
    if (length == 4) return (static_cast<std::uint32_t>(uc & 0x07) << 18)
                          | (static_cast<std::uint32_t>(bytes[1] & 0x3F) << 12)
                          | (static_cast<std::uint32_t>(bytes[2] & 0x3F) << 6)
                          | static_cast<std::uint32_t>(bytes[3] & 0x3F);
    return 0;
}

Utf8Result validate_utf8(std::string_view input, std::size_t start_offset) {
    auto size = input.size();
    for (auto i = start_offset; i < size; ) {
        auto uc = static_cast<unsigned char>(input[i]);
        if (uc <= 0x7F) {
            if (uc == 0x00) {
                std::size_t null_count = 0;
                while (i + null_count < size && input[i + null_count] == '\0') ++null_count;
                return {false, i, "Null byte (0x00) is not valid in source text"};
            }
            ++i;
            continue;
        }
        if ((uc & 0xC0) == 0x80) {
            return {false, i, "Isolated continuation byte"};
        }
        std::size_t seq_len;
        if ((uc & 0xE0) == 0xC0) seq_len = 2;
        else if ((uc & 0xF0) == 0xE0) seq_len = 3;
        else if ((uc & 0xF8) == 0xF0) seq_len = 4;
        else {
            return {false, i, "Invalid lead byte"};
        }
        if (i + seq_len > size) {
            return {false, i, "Truncated UTF-8 sequence"};
        }
        for (std::size_t j = 1; j < seq_len; ++j) {
            if (!is_utf8_continuation_byte(input[i + j])) {
                return {false, i, "Missing continuation byte in UTF-8 sequence"};
            }
        }
        auto cp = utf8_codepoint(&input[i], seq_len);
        if (cp > 0x10FFFF) {
            return {false, i, "Code point exceeds U+10FFFF"};
        }
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            return {false, i, "Surrogate code point (U+D800-U+DFFF) is not valid UTF-8"};
        }
        if (seq_len == 2 && cp < 0x80) {
            return {false, i, "Overlong 2-byte sequence"};
        }
        if (seq_len == 3 && cp < 0x800) {
            return {false, i, "Overlong 3-byte sequence"};
        }
        if (seq_len == 4 && cp < 0x10000) {
            return {false, i, "Overlong 4-byte sequence"};
        }
        i += seq_len;
    }
    return {true, 0, ""};
}

bool has_bom(std::string const& input) {
    return input.size() >= 3
        && static_cast<unsigned char>(input[0]) == 0xEF
        && static_cast<unsigned char>(input[1]) == 0xBB
        && static_cast<unsigned char>(input[2]) == 0xBF;
}

std::string strip_bom(std::string input) {
    if (has_bom(input)) {
        input.erase(0, 3);
    }
    return input;
}

std::size_t utf8_column_advance(std::string const& text, std::size_t offset) {
    if (offset >= text.size()) return 0;
    auto uc = static_cast<unsigned char>(text[offset]);
    if (uc <= 0x7F) return 1;
    auto len = utf8_sequence_length(text[offset]);
    if (len == 0) return 1;
    return len;
}

UnicodeCategory classify_utf8_byte(char c) {
    auto uc = static_cast<unsigned char>(c);
    if (uc <= 0x7F) {
        if (std::isalpha(uc)) return UnicodeCategory::LETTER;
        if (std::isdigit(uc)) return UnicodeCategory::DIGIT;
        if (uc == '_') return UnicodeCategory::UNDERSCORE;
        if (uc == ' ' || uc == '\t' || uc == '\n' || uc == '\r') return UnicodeCategory::WHITESPACE;
        return UnicodeCategory::OPERATOR;
    }
    if ((uc & 0xC0) == 0x80) return UnicodeCategory::CONTINUATION;
    return UnicodeCategory::LETTER;
}

} // namespace gspl
