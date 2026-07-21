#pragma once
#include "gspl/source.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace gspl {

enum class DiagnosticSeverity : std::uint8_t { note, warning, error, fatal };

enum class DiagnosticCode : std::uint32_t {
    GSPL_LEX_INVALID_CHARACTER = 1001,
    GSPL_LEX_UNTERMINATED_STRING = 1002,
    GSPL_LEX_UNSUPPORTED_ESCAPE = 1003,
    GSPL_LEX_NUMERIC_OVERFLOW = 1004,
    GSPL_LEX_INVALID_UTF8 = 1005,
    GSPL_LEX_OVERSIZED_TOKEN = 1006,
    GSPL_PARSE_UNEXPECTED_TOKEN = 2001,
    GSPL_PARSE_MISSING_SEMICOLON = 2002,
    GSPL_PARSE_UNBALANCED_BRACE = 2003,
    GSPL_PARSE_INVALID_DECLARATION = 2004,
    GSPL_PARSE_NESTING_EXCEEDED = 2005,
    GSPL_MODULE_DUPLICATE = 3001,
    GSPL_MODULE_IMPORT_CYCLE = 3002,
    GSPL_MODULE_UNRESOLVED = 3003,
    GSPL_MODULE_PATH_TRAVERSAL = 3004,
    GSPL_NAME_DUPLICATE = 4001,
    GSPL_NAME_UNKNOWN = 4002,
    GSPL_NAME_AMBIGUOUS = 4003,
    GSPL_NAME_WRONG_CATEGORY = 4004,
    GSPL_TYPE_MISMATCH = 5001,
    GSPL_TYPE_INCOMPATIBLE_UNITS = 5002,
    GSPL_TYPE_INVALID_CONVERSION = 5003,
    GSPL_GENE_DUPLICATE = 6001,
    GSPL_GENE_CONFLICT = 6002,
    GSPL_GENE_MISSING_DEPENDENCY = 6003,
    GSPL_CONSTRAINT_UNSATISFIED = 7001,
    GSPL_IR_UNSUPPORTED_VERSION = 8001,
    GSPL_IR_VALIDATION_ERROR = 8002,
    GSPL_PASS_CYCLE = 9001,
    GSPL_PASS_MISSING_DEPENDENCY = 9002,
    GSPL_PASS_DUPLICATE = 9000,
    GSPL_PASS_VERSION_MISMATCH = 9003,
    GSPL_RESOURCE_EXCEEDED = 10001,
    GSPL_RIGHTS_VIOLATION = 11001,
    GSPL_PROVENANCE_MISMATCH = 12001,
};

struct Diagnostic {
    DiagnosticCode code{};
    DiagnosticSeverity severity{DiagnosticSeverity::error};
    std::string message;
    SourceSpan span;
    std::vector<SourceSpan> related_spans;
    std::string remediation;
};

struct DiagnosticResult {
    std::vector<Diagnostic> diagnostics;
    bool ok() const { return diagnostics.empty(); }
    void add(Diagnostic d) { diagnostics.push_back(std::move(d)); }
    void add_error(DiagnosticCode code, std::string msg, SourceSpan span) {
        diagnostics.push_back({code, DiagnosticSeverity::error, std::move(msg), span, {}, {}});
    }
    std::string to_json() const;
    void sort() {
        std::sort(diagnostics.begin(), diagnostics.end(), [](auto const& a, auto const& b) {
            return std::tuple(a.span.byte_offset, static_cast<int>(a.code)) <
                   std::tuple(b.span.byte_offset, static_cast<int>(b.code));
        });
    }
};

} // namespace gspl
