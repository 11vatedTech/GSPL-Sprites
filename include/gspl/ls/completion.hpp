#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace gspl::ls {

enum class CompletionItemKind {
    Keyword = 14,
    Module = 9,
    Class = 5,
    Field = 9,
    Function = 2,
    Variable = 6,
    Property = 10,
    Snippet = 15
};

struct CompletionItem {
    std::string label;
    CompletionItemKind kind{CompletionItemKind::Keyword};
    std::string detail;
    std::string insert_text;
    std::string documentation;
};

struct CompletionList {
    std::vector<CompletionItem> items;
    bool is_incomplete{false};
    void filter(std::string_view prefix);
};

std::string completion_item_to_json(const CompletionItem& item);
std::string completion_list_to_json(const CompletionList& list);
} // namespace gspl::ls
