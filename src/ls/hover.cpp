#include "gspl/ls/hover.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/parser.hpp"
#include <sstream>

namespace gspl::ls {

static const char* keyword_doc(std::string_view keyword) {
    static const struct { const char* kw; const char* doc; } docs[] = {
        {"module", "Declare a module — the top-level unit of GSPL source organization."},
        {"entity", "Declare an entity — a living sprite with genes, morphology, forms, and behavior."},
        {"gene", "Declare a Sprite Gene — a typed capability contract for an entity."},
        {"form", "Declare a form — a visual/material configuration of an entity."},
        {"morphology", "Declare morphology — the structural composition of parts and joints."},
        {"animation", "Declare an animation clip or state machine."},
        {"behavior", "Declare behavior rules — condition/action pairs for autonomous decision-making."},
        {"combat", "Declare combat capabilities — abilities, projectiles, collisions, combos."},
        {"import", "Import symbols from another module."},
        {"from", "Specify the source module in an import statement."},
        {"as", "Create an alias for an imported symbol."},
        {"let", "Declare a local immutable binding."},
        {"fn", "Declare a function."},
        {"if", "Conditional execution."},
        {"else", "Alternative branch in conditional execution."},
        {"match", "Pattern matching against enumerated values."},
        {"return", "Return a value from a function."},
        {"true", "Boolean literal: true."},
        {"false", "Boolean literal: false."},
        {"null", "Null value for optional types and references."},
        {"Int", "Signed integer type (bounded, deterministic)."},
        {"UInt", "Unsigned integer type (bounded, deterministic)."},
        {"Fixed", "Fixed-point decimal type (bounded, deterministic)."},
        {"Bool", "Boolean type (true/false)."},
        {"String", "UTF-8 string type."},
        {"Color", "RGBA color type with 8-bit channels."},
        {"Vector2", "2D vector type with Fixed components."},
        {"Vector3", "3D vector type with Fixed components."},
        {"Duration", "Time duration type (Fixed, ticks)."},
        {"Distance", "Spatial distance type (Fixed, pixels/units)."},
        {"Angle", "Angular measurement type (Fixed, degrees)."},
        {"Percentage", "Proportional type (Fixed, 0.0–1.0)."},
        {"Ratio", "Dimensionless ratio type (Fixed)."},
    };
    for (auto& d : docs) {
        if (d.kw == keyword) return d.doc;
    }
    return nullptr;
}

HoverInfo get_hover_info(std::string_view source, std::string_view uri,
                         std::uint32_t line, std::uint32_t column) {
    HoverInfo info;

    auto buf = gspl::SourceBuffer::from_string(std::string(uri), std::string(source));
    gspl::SourceManager sources;
    auto buf_id = sources.register_buffer(std::move(buf));
    auto* sb = sources.lookup(static_cast<gspl::SourceId>(buf_id));
    if (!sb) return info;

    gspl::LexerConfig lex_cfg;
    gspl::Lexer lexer(*sb, lex_cfg);
    auto tokens = lexer.tokenize();

    // Find token at cursor position
    std::uint64_t cursor_offset = sb->offset_for_location({line, column});
    std::string_view matched_text;
    Range matched_range;

    for (const auto& tok : tokens) {
        if (tok.span.byte_offset <= cursor_offset &&
            cursor_offset < tok.span.byte_offset + tok.span.byte_length) {
            auto* tok_buf = sources.lookup(tok.span.source_id);
            if (tok_buf) {
                matched_text = tok_buf->content().substr(
                    tok.span.byte_offset, tok.span.byte_length);
            }
            matched_range.start.line = tok.span.start.line;
            matched_range.start.column = tok.span.start.column;
            matched_range.end.line = tok.span.end.line;
            matched_range.end.column = tok.span.end.column;
            break;
        }
    }

    if (matched_text.empty()) {
        // Find nearest token after cursor
        for (const auto& tok : tokens) {
            if (tok.span.byte_offset >= cursor_offset) {
                auto* tok_buf = sources.lookup(tok.span.source_id);
                if (tok_buf) {
                    matched_text = tok_buf->content().substr(
                        tok.span.byte_offset, tok.span.byte_length);
                }
                matched_range.start.line = tok.span.start.line;
                matched_range.start.column = tok.span.start.column;
                matched_range.end.line = tok.span.end.line;
                matched_range.end.column = tok.span.end.column;
                break;
            }
        }
    }

    if (matched_text.empty()) return info;

    // Check keywords first
    if (auto* doc = keyword_doc(matched_text)) {
        std::ostringstream oss;
        oss << "**" << matched_text << "**  \n" << doc;
        info.contents = oss.str();
        info.range = matched_range;
        return info;
    }

    // Try to find declaration info via parser
    gspl::Parser parser(tokens, sources);
    auto module = parser.parse_module();
    if (module) {
        auto decl_name = [](gspl::AstNode const* n) -> std::string_view {
            using enum gspl::AstKind;
            switch (n->kind) {
            case gspl::AstKind::entity:        return static_cast<gspl::EntityDecl const*>(n)->name;
            case gspl::AstKind::gene:          return static_cast<gspl::GeneDecl const*>(n)->name;
            case gspl::AstKind::form:          return static_cast<gspl::FormDecl const*>(n)->name;
            case gspl::AstKind::transformation:return static_cast<gspl::TransformationDecl const*>(n)->name;
            case gspl::AstKind::part:          return static_cast<gspl::PartDecl const*>(n)->name;
            case gspl::AstKind::resource:      return static_cast<gspl::ResourceDecl const*>(n)->name;
            case gspl::AstKind::ability:       return static_cast<gspl::AbilityDecl const*>(n)->name;
            default: return {};
            }
        };
        for (const auto& decl : module->declarations) {
            auto name = decl_name(decl.get());
            if (name == matched_text) {
                std::ostringstream oss;
                oss << "**" << name << "**  \n`" << uri << "`  \n"
                    << "Line " << (decl->span.start.line + 1);
                info.contents = oss.str();
                info.range = matched_range;
                return info;
            }
        }
    }

    // Fallback: generic identifier info
    std::ostringstream oss;
    oss << "`" << matched_text << "`";
    info.contents = oss.str();
    info.range = matched_range;
    return info;
}

std::string hover_info_to_json(const HoverInfo& info) {
    std::ostringstream oss;
    oss << "{\"contents\":{\"kind\":\"markdown\",\"value\":\""
        << info.contents << "\"}";
    if (info.range.start.line != 0 || info.range.end.line != 0) {
        oss << ",\"range\":{"
            << "\"start\":{\"line\":" << info.range.start.line
            << ",\"character\":" << info.range.start.column << "},"
            << "\"end\":{\"line\":" << info.range.end.line
            << ",\"character\":" << info.range.end.column << "}}";
    }
    oss << "}";
    return oss.str();
}

} // namespace gspl::ls
