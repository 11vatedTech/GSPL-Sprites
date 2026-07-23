#pragma once

#include "gspl/diagnostics.hpp"
#include <string>
#include <cstdint>
#include <vector>

namespace gspl::ls {

struct Position {
    std::uint32_t line{0};
    std::uint32_t column{0};
};

struct Range {
    Position start;
    Position end;
};

enum class DiagnosticSeverity {
    Hint = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity{DiagnosticSeverity::Error};
    std::string message;
    std::string code;
    std::string source{"gspl-ls"};
};

struct DiagnosticBatch {
    std::string document_uri;
    std::vector<Diagnostic> diagnostics;
};

std::string diagnostic_to_json(const Diagnostic& diag);
Diagnostic from_compile_error(const gspl::Diagnostic& compile_err, const std::string& source_name);
} // namespace gspl::ls
