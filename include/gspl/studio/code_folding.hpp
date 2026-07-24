#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

struct FoldRegion {
    size_t start_line = 0;
    size_t end_line = 0;
    std::string preview;  // text shown when folded
    bool collapsed = false;
};

class CodeFoldingModel {
public:
    explicit CodeFoldingModel(std::string_view text);

    // Analyze text and find foldable regions
    std::vector<FoldRegion> analyze() const;

    // Toggle collapse state
    void toggle(size_t line);

    // Check if a line is hidden due to a collapsed fold above it
    bool is_hidden(size_t line) const;

    // Get visible line number (accounting for collapsed folds)
    size_t visible_line(size_t line) const;

    // Clear all fold state
    void reset();

    std::vector<FoldRegion> const& regions() const { return regions_; }

private:
    std::string text_;
    std::vector<FoldRegion> regions_;

    // Find brace-delimited fold regions
    void find_brace_regions();
};

} // namespace gspl::studio
