#include "gspl/studio/search_engine.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

namespace gspl::studio {

SearchEngine::SearchEngine(std::string text) : text_(std::move(text)) {}

bool SearchEngine::matches_at(std::string_view text, size_t pos, SearchQuery const& query) {
    if (pos + query.pattern.size() > text.size()) return false;

    if (query.use_regex) {
        try {
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!query.case_sensitive) flags |= std::regex::icase;
            std::regex re(query.pattern, flags);
            std::cmatch m;
            auto start = text.data() + pos;
            if (std::regex_search(start, start + query.pattern.size(), m, re, std::regex_constants::match_continuous)) {
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    for (size_t i = 0; i < query.pattern.size(); ++i) {
        char tc = text[pos + i];
        char pc = query.pattern[i];
        if (!query.case_sensitive) {
            tc = static_cast<char>(std::tolower(static_cast<unsigned char>(tc)));
            pc = static_cast<char>(std::tolower(static_cast<unsigned char>(pc)));
        }
        if (tc != pc) return false;
    }

    if (query.whole_word) {
        if (pos > 0 && is_word_boundary(text, pos - 1)) return false;
        if (pos + query.pattern.size() < text.size() && is_word_boundary(text, pos + query.pattern.size())) return false;
    }

    return true;
}

bool SearchEngine::is_word_boundary(std::string_view text, size_t pos) {
    if (pos >= text.size()) return true;
    char c = text[pos];
    return !std::isalnum(static_cast<unsigned char>(c)) && c != '_';
}

std::vector<SearchMatch> SearchEngine::find_all(SearchQuery const& query) const {
    std::vector<SearchMatch> matches;
    if (query.pattern.empty()) return matches;

    size_t line = 0;
    size_t line_start = 0;

    for (size_t i = 0; i <= text_.size(); ++i) {
        if (i == text_.size() || text_[i] == '\n') {
            // Search within this line
            std::string_view line_view(text_.data() + line_start, i - line_start);
            for (size_t col = 0; col + query.pattern.size() <= line_view.size(); ++col) {
                if (matches_at(text_, line_start + col, query)) {
                    SearchMatch m;
                    m.line = line;
                    m.column = col;
                    m.length = query.pattern.size();
                    m.text = std::string(text_.substr(line_start + col, query.pattern.size()));
                    m.line_text = std::string(line_view);
                    matches.push_back(m);
                }
            }
            line_start = i + 1;
            ++line;
        }
    }

    return matches;
}

std::optional<SearchMatch> SearchEngine::find_next(SearchQuery const& query, size_t cursor_pos) const {
    auto all = find_all(query);
    auto compute_pos = [&](SearchMatch const& m) -> size_t {
        size_t pos = 0;
        for (size_t l = 0; l < m.line; ++l) {
            auto nl = text_.find('\n', pos);
            if (nl == std::string_view::npos) break;
            pos = nl + 1;
        }
        return pos + m.column;
    };
    for (auto const& m : all) {
        if (compute_pos(m) > cursor_pos) return m;
    }
    return std::nullopt;
}

std::optional<SearchMatch> SearchEngine::find_prev(SearchQuery const& query, size_t cursor_pos) const {
    auto all = find_all(query);
    std::optional<SearchMatch> result;
    auto compute_pos = [&](SearchMatch const& m) -> size_t {
        size_t pos = 0;
        for (size_t l = 0; l < m.line; ++l) {
            auto nl = text_.find('\n', pos);
            if (nl == std::string_view::npos) break;
            pos = nl + 1;
        }
        return pos + m.column;
    };
    for (auto const& m : all) {
        if (compute_pos(m) >= cursor_pos) break;
        result = m;
    }
    return result;
}

ReplaceResult SearchEngine::replace_all(SearchQuery const& query, std::string_view replacement) {
    ReplaceResult result;
    result.text = text_;
    if (query.pattern.empty()) return result;

    size_t pos = 0;
    std::string output;

    while (pos < result.text.size()) {
        if (matches_at(result.text, pos, query)) {
            output += replacement;
            pos += query.pattern.size();
            ++result.replacements;
        } else {
            output += result.text[pos];
            ++pos;
        }
    }

    result.text = output;
    return result;
}

SearchEngine::IncrementalResult SearchEngine::incremental_search(std::string_view prefix, bool case_sensitive) const {
    IncrementalResult result;
    SearchQuery q;
    q.pattern = std::string(prefix);
    q.case_sensitive = case_sensitive;
    result.matches = find_all(q);
    result.match_count = result.matches.size();
    if (!result.matches.empty())
        result.current_match = result.matches[0];
    return result;
}

} // namespace gspl::studio
