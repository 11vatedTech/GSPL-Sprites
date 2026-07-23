#include "gspl/ls/diagnostic.hpp"
#include "gspl/diagnostics.hpp"
#include <sstream>

namespace gspl::ls {

std::string diagnostic_severity_string(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Hint: return "hint";
        case DiagnosticSeverity::Info: return "info";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Error: return "error";
    }
    return "unknown";
}

std::string diagnostic_to_json(const Diagnostic& diag) {
    std::ostringstream oss;
    oss << "{\"range\":{"
        << "\"start\":{\"line\":" << diag.range.start.line
        << ",\"character\":" << diag.range.start.column << "},"
        << "\"end\":{\"line\":" << diag.range.end.line
        << ",\"character\":" << diag.range.end.column << "}"
        << "},\"severity\":" << static_cast<int>(diag.severity)
        << ",\"message\":\"" << diag.message << "\""
        << ",\"source\":\"" << diag.source << "\"}";
    return oss.str();
}

Diagnostic from_compile_error(const gspl::Diagnostic& compile_err, const std::string&) {
    Diagnostic diag;
    diag.range.start.line = compile_err.span.start.line;
    diag.range.start.column = compile_err.span.start.column;
    diag.range.end.line = compile_err.span.end.line;
    diag.range.end.column = compile_err.span.end.column;
    diag.severity = compile_err.severity == gspl::DiagnosticSeverity::error
        ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
    diag.message = compile_err.message;
    diag.code = std::to_string(static_cast<std::uint32_t>(compile_err.code));
    return diag;
}

} // namespace gspl::ls
