#include "gspl/utf8.hpp"
#include "gspl/gspl.hpp"
#include "gspl/passes.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
    try {
        // ---- 1. Valid ASCII ----
        {
            std::string ascii = "hello world";
            auto r = gspl::validate_utf8(ascii, 0);
            check(r.valid, "ASCII should be valid UTF-8");
        }

        // ---- 2. Valid multibyte UTF-8 ----
        {
            // café in UTF-8: 0x63 0x61 0x66 0xC3 0xA9
            std::string mb = "caf\xC3\xA9";
            auto r = gspl::validate_utf8(mb, 0);
            check(r.valid, "Multibyte UTF-8 should be valid");

            // 中文 in UTF-8: 0xE4 0xB8 0xAD 0xE6 0x96 0x87
            std::string cjk = "\xE4\xB8\xAD\xE6\x96\x87";
            r = gspl::validate_utf8(cjk, 0);
            check(r.valid, "CJK UTF-8 should be valid");
        }

        // ---- 3. Emoji (4-byte) ----
        {
            // 😀 in UTF-8: 0xF0 0x9F 0x98 0x80
            std::string emoji = "\xF0\x9F\x98\x80";
            auto r = gspl::validate_utf8(emoji, 0);
            check(r.valid, "Emoji (4-byte UTF-8) should be valid");
        }

        // ---- 4. Combining characters ----
        {
            // é (U+00E9) + combining acute (U+0301)
            std::string combined = "\xC3\xA9\xCC\x81";
            auto r = gspl::validate_utf8(combined, 0);
            check(r.valid, "Combining characters should be valid UTF-8");
        }

        // ---- 5. BOM detection and stripping ----
        {
            std::string with_bom = std::string("\xEF\xBB\xBF") + "hello";
            check(gspl::has_bom(with_bom), "Should detect BOM");
            auto stripped = gspl::strip_bom(with_bom);
            check(stripped == "hello", "BOM should be stripped");
            check(!stripped.empty(), "Stripped content should not be empty");
            check(!gspl::has_bom(stripped), "Stripped should not have BOM");

            std::string no_bom = "hello";
            check(!gspl::has_bom(no_bom), "No BOM should not be detected");
            auto same = gspl::strip_bom(no_bom);
            check(same == "hello", "strip_bom on non-BOM should be identity");
        }

        // ---- 6. Overlong encoding ----
        {
            // Overlong 2-byte encoding of '/' (0x2F)
            std::string overlong = "\xC0\xAF";
            auto r = gspl::validate_utf8(overlong, 0);
            check(!r.valid, "Overlong 2-byte should be invalid");
            check(r.message.find("Overlong") != std::string::npos ||
                   r.message.find("Invalid") != std::string::npos, "Should mention overlong");

            // Overlong 3-byte encoding of 'A' (0x41)
            std::string overlong3 = "\xE0\x81\x81";
            r = gspl::validate_utf8(overlong3, 0);
            check(!r.valid, "Overlong 3-byte should be invalid");

            // Overlong 4-byte encoding of space
            std::string overlong4 = "\xF0\x80\x80\x20";
            r = gspl::validate_utf8(overlong4, 0);
            check(!r.valid, "Overlong 4-byte should be invalid");
        }

        // ---- 7. Isolated continuation bytes ----
        {
            std::string isolated = "\x80\xBF\x80";
            auto r = gspl::validate_utf8(isolated, 0);
            check(!r.valid, "Isolated continuation bytes should be invalid");
        }

        // ---- 8. Truncated sequences ----
        {
            std::string truncated = "\xE2\x82";  // truncated 3-byte euro sign
            auto r = gspl::validate_utf8(truncated, 0);
            check(!r.valid, "Truncated 3-byte sequence should be invalid");

            std::string truncated4 = "\xF0\x9F\x98";  // truncated 4-byte
            r = gspl::validate_utf8(truncated4, 0);
            check(!r.valid, "Truncated 4-byte sequence should be invalid");
        }

        // ---- 9. Surrogate encodings (UTF-8 encoding of surrogate halves) ----
        {
            std::string surrogate = "\xED\xA0\x80";  // U+D800
            auto r = gspl::validate_utf8(surrogate, 0);
            check(!r.valid, "Surrogate (U+D800) should be invalid in UTF-8");

            std::string surrogate2 = "\xED\xBF\xBF";  // U+DFFF
            r = gspl::validate_utf8(surrogate2, 0);
            check(!r.valid, "Surrogate (U+DFFF) should be invalid in UTF-8");
        }

        // ---- 10. Invalid code points (> U+10FFFF) ----
        {
            std::string beyond = "\xF4\x90\x80\x80";  // U+110000
            auto r = gspl::validate_utf8(beyond, 0);
            check(!r.valid, "Code point beyond U+10FFFF should be invalid");
        }

        // ---- 11. Null bytes ----
        {
            std::string with_null("hello", 5);
            with_null.push_back('\0');
            with_null.append("world", 5);
            auto r = gspl::validate_utf8(with_null, 0);
            check(!r.valid, "Null bytes should be invalid");
        }

        // ---- 12. Invalid lead bytes (0xFE, 0xFF) ----
        {
            std::string bad_lead = "\xFE";
            auto r = gspl::validate_utf8(bad_lead, 0);
            check(!r.valid, "0xFE should be invalid lead byte");

            bad_lead = "\xFF";
            r = gspl::validate_utf8(bad_lead, 0);
            check(!r.valid, "0xFF should be invalid lead byte");
        }

        // ---- 13. utf8_sequence_length ----
        {
            check(gspl::utf8_sequence_length('A') == 1, "ASCII sequence length should be 1");
            check(gspl::utf8_sequence_length(static_cast<char>(0xC2)) == 2, "Lead 0xC2 length should be 2");
            check(gspl::utf8_sequence_length(static_cast<char>(0xE2)) == 3, "Lead 0xE2 length should be 3");
            check(gspl::utf8_sequence_length(static_cast<char>(0xF0)) == 4, "Lead 0xF0 length should be 4");
            check(gspl::utf8_sequence_length(static_cast<char>(0x80)) == 0, "Continuation byte length should be 0");
            check(gspl::utf8_sequence_length(static_cast<char>(0xFE)) == 0, "Invalid lead 0xFE length should be 0");
        }

        // ---- 14. is_utf8_continuation_byte ----
        {
            check(!gspl::is_utf8_continuation_byte('A'), "ASCII should not be continuation");
            check(gspl::is_utf8_continuation_byte(static_cast<char>(0x80)), "0x80 should be continuation");
            check(gspl::is_utf8_continuation_byte(static_cast<char>(0xBF)), "0xBF should be continuation");
        }

        // ---- 15. utf8_codepoint ----
        {
            // U+00E9 (é) in UTF-8: 0xC3 0xA9
            char cafe[] = {static_cast<char>(0xC3), static_cast<char>(0xA9)};
            auto cp = gspl::utf8_codepoint(cafe, 2);
            check(cp == 0xE9, "Codepoint for é should be U+00E9");

            // U+4E2D (中)
            char cjk[] = {static_cast<char>(0xE4), static_cast<char>(0xB8), static_cast<char>(0xAD)};
            cp = gspl::utf8_codepoint(cjk, 3);
            check(cp == 0x4E2D, "Codepoint for 中 should be U+4E2D");

            // U+1F600 (😀)
            char emoji[] = {static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x98), static_cast<char>(0x80)};
            cp = gspl::utf8_codepoint(emoji, 4);
            check(cp == 0x1F600, "Codepoint for 😀 should be U+1F600");
        }

        // ---- 16. classify_utf8_byte ----
        {
            check(gspl::classify_utf8_byte('A') == gspl::UnicodeCategory::LETTER, "'A' should be LETTER");
            check(gspl::classify_utf8_byte('z') == gspl::UnicodeCategory::LETTER, "'z' should be LETTER");
            check(gspl::classify_utf8_byte('0') == gspl::UnicodeCategory::DIGIT, "'0' should be DIGIT");
            check(gspl::classify_utf8_byte('_') == gspl::UnicodeCategory::UNDERSCORE, "'_' should be UNDERSCORE");
            check(gspl::classify_utf8_byte(' ') == gspl::UnicodeCategory::WHITESPACE, "' ' should be WHITESPACE");
            check(gspl::classify_utf8_byte('\n') == gspl::UnicodeCategory::WHITESPACE, "'\\n' should be WHITESPACE");
            check(gspl::classify_utf8_byte('+') == gspl::UnicodeCategory::OPERATOR, "'+' should be OPERATOR");
            check(gspl::classify_utf8_byte(static_cast<char>(0x80)) == gspl::UnicodeCategory::CONTINUATION, "0x80 should be CONTINUATION");
            check(gspl::classify_utf8_byte(static_cast<char>(0xC2)) == gspl::UnicodeCategory::LETTER, "0xC2 should be LETTER");
        }

        // ---- 17. Invalid bytes at start, middle, end of file ----
        {
            std::string start_bad = "\x80hello";
            auto r = gspl::validate_utf8(start_bad, 0);
            check(!r.valid, "Continuation byte at start should be invalid");

            std::string mid_bad = "he\x80llo";
            r = gspl::validate_utf8(mid_bad, 0);
            check(!r.valid, "Continuation byte in middle should be invalid");

            std::string end_bad = "hello\x80";
            r = gspl::validate_utf8(end_bad, 0);
            check(!r.valid, "Continuation byte at end should be invalid");
        }

        // ---- 18. Lexer rejects invalid UTF-8 ----
        {
            std::string bad_src = std::string("module test;\nentity X {\nrights ORIGINAL_USER_CREATION PUBLIC;\nmorphology {}\n") + char(0x80) + char(0x80) + ";\n}\n";
            auto buf = gspl::SourceBuffer::from_string("bad_utf8.gspl", bad_src);
            gspl::CompilationContext ctx;
            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex;
            lex.execute(ctx);
            bool found_invalid_utf8 = false;
            for (auto const& d : ctx.diagnostics.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_LEX_INVALID_UTF8) {
                    found_invalid_utf8 = true;
                    break;
                }
            }
            check(found_invalid_utf8, "Lexer should emit GSPL_LEX_INVALID_UTF8 for invalid bytes");
        }

        // ---- 19. Lexer accepts valid multibyte identifiers ----
        {
            // Use a module name with valid UTF-8 in comments (identifiers still ASCII for GSPL)
            auto buf = gspl::SourceBuffer::from_string("utf8_ok.gspl",
                "module utf8test;\n"
                "entity X {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                // valid multibyte in comment
                "  // café 中文\n"
                "  morphology {}\n"
                "}\n");
            gspl::CompilationContext ctx;
            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex;
            auto diags = lex.execute(ctx);
            bool has_utf8_error = false;
            for (auto const& d : diags.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_LEX_INVALID_UTF8) {
                    has_utf8_error = true;
                }
            }
            check(!has_utf8_error, "Valid multibyte in comments should not produce UTF-8 errors");
            check(ctx.ast == nullptr || !has_utf8_error, "Parse should not produce errors about valid UTF-8");
        }

        // ---- 20. BOM is handled by from_file (from_string passes through as-is) ----
        {
            std::string bom_bytes = "\xEF\xBB\xBF";
            auto content = bom_bytes + "module bomtest;\nentity X { rights PUBLIC; morphology {} }\n";
            auto buf = gspl::SourceBuffer::from_string("bom.gspl", content);
            // from_string does not strip BOM (from_file does)
            check(buf.content().substr(0, 3) == bom_bytes,
                  "SourceBuffer from_string should preserve BOM");
        }

        // ---- 21. utf8_column_advance ----
        {
            std::string text = "abc";
            check(gspl::utf8_column_advance(text, 0) == 1, "ASCII column advance should be 1");

            // é as 2 bytes
            std::string mb = "\xC3\xA9";
            check(gspl::utf8_column_advance(mb, 0) == 2, "2-byte UTF-8 column advance should be 2");

            // 中 as 3 bytes
            std::string cjk = "\xE4\xB8\xAD";
            check(gspl::utf8_column_advance(cjk, 0) == 3, "3-byte UTF-8 column advance should be 3");

            // 😀 as 4 bytes
            std::string emoji = "\xF0\x9F\x98\x80";
            check(gspl::utf8_column_advance(emoji, 0) == 4, "4-byte UTF-8 column advance should be 4");
        }

        std::cout << "ALL UTF-8 TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
