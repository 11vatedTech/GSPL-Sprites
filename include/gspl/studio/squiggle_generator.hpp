#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

enum class SquiggleSeverity {
    Hint,
    Info,
    Warning,
    Error,
};

struct SquiggleRange {
    size_t line = 0;
    size_t start_col = 0;
    size_t end_col = 0;
    SquiggleSeverity severity = SquiggleSeverity::Error;
    std::string message;
    std::string code;         // e.g. "E001"
    std::vector<std::string> quick_fixes;  // suggestion labels
};

using DiagnosticSource = std::function<std::vector<SquiggleRange>()>;

class SquiggleGenerator {
public:
    SquiggleGenerator();

    // Register a source of diagnostics (e.g. compiler, language service)
    void add_source(DiagnosticSource source);

    // Collect all squiggles from all sources
    std::vector<SquiggleRange> generate();

    // Clear all sources
    void clear_sources();

    // Get squiggles for a specific line (for rendering)
    std::vector<SquiggleRange> for_line(std::vector<SquiggleRange> const& all, size_t line) const;

    // Convert a compiler diagnostic format to squiggle format
    // This is a stub that would integrate with the real diagnostic pipeline
    static SquiggleRange from_diagnostic(int code, std::string_view message,
                                          size_t line, size_t col, size_t length);

private:
    std::vector<DiagnosticSource> sources_;
};

} // namespace gspl::studio
