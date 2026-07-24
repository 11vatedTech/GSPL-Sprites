#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>

namespace gspl::studio {

struct Snippet {
    std::string prefix;
    std::string description;
    std::string body;       // snippet text with ${1:placeholder} syntax
    std::string scope;      // language scope (e.g. "gspl")
};

struct SnippetTabStop {
    size_t index = 0;
    size_t start = 0;
    size_t end = 0;
    std::string default_value;
    bool is_placeholder = true;
};

struct SnippetExpansion {
    std::string text;
    std::vector<SnippetTabStop> tab_stops;
    size_t end_cursor_pos = 0;
};

class SnippetEngine {
public:
    SnippetEngine();

    void add_snippet(Snippet snip);
    void add_builtins();
    void clear();

    // Find snippets matching prefix
    std::vector<Snippet const*> find(std::string_view prefix) const;

    // Expand a snippet's body (interpolate $1, ${1:default}, etc.)
    static SnippetExpansion expand(std::string_view body);

    // Get all registered snippets
    std::vector<Snippet> const& all() const { return snippets_; }

private:
    std::vector<Snippet> snippets_;
};

} // namespace gspl::studio
