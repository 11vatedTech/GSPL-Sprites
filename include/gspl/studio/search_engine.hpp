#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <optional>

namespace gspl::studio {

struct SearchMatch {
    size_t line = 0;
    size_t column = 0;
    size_t length = 0;
    std::string text;
    std::string line_text;
};

struct SearchQuery {
    std::string pattern;
    bool case_sensitive = false;
    bool whole_word = false;
    bool use_regex = false;
};

struct ReplaceResult {
    std::string text;
    size_t replacements = 0;
};

class SearchEngine {
public:
    explicit SearchEngine(std::string text);

    // Find all matches for query
    std::vector<SearchMatch> find_all(SearchQuery const& query) const;

    // Find next/previous match from cursor position
    std::optional<SearchMatch> find_next(SearchQuery const& query, size_t cursor_pos) const;
    std::optional<SearchMatch> find_prev(SearchQuery const& query, size_t cursor_pos) const;

    // Replace all occurrences
    ReplaceResult replace_all(SearchQuery const& query, std::string_view replacement);

    // Incremental search - returns current match count and highlight positions
    struct IncrementalResult {
        size_t match_count = 0;
        std::vector<SearchMatch> matches;
        std::optional<SearchMatch> current_match;
    };
    IncrementalResult incremental_search(std::string_view prefix, bool case_sensitive = false) const;

    // Access text
    std::string_view text() const { return text_; }

private:
    std::string text_;

    static bool matches_at(std::string_view text, size_t pos, SearchQuery const& query);
    static bool is_word_boundary(std::string_view text, size_t pos);
};

} // namespace gspl::studio
