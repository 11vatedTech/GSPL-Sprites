#include "gspl/studio/squiggle_generator.hpp"

namespace gspl::studio {

SquiggleGenerator::SquiggleGenerator() = default;

void SquiggleGenerator::add_source(DiagnosticSource source) {
    sources_.push_back(std::move(source));
}

std::vector<SquiggleRange> SquiggleGenerator::generate() {
    std::vector<SquiggleRange> all;
    for (auto const& source : sources_) {
        auto ranges = source();
        all.insert(all.end(), ranges.begin(), ranges.end());
    }
    return all;
}

void SquiggleGenerator::clear_sources() {
    sources_.clear();
}

std::vector<SquiggleRange> SquiggleGenerator::for_line(std::vector<SquiggleRange> const& all, size_t line) const {
    std::vector<SquiggleRange> result;
    for (auto const& sq : all) {
        if (sq.line == line) result.push_back(sq);
    }
    return result;
}

SquiggleRange SquiggleGenerator::from_diagnostic(int code, std::string_view message,
                                                   size_t line, size_t col, size_t length) {
    SquiggleRange sq;
    sq.line = line;
    sq.start_col = col;
    sq.end_col = col + length;
    sq.severity = (code >= 5000) ? SquiggleSeverity::Error : SquiggleSeverity::Warning;
    sq.message = std::string(message);
    sq.code = "E" + std::to_string(code);
    return sq;
}

} // namespace gspl::studio
