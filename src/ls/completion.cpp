#include "gspl/ls/completion.hpp"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace gspl::ls {

std::string completion_item_to_json(const CompletionItem& item) {
    std::ostringstream oss;
    oss << "{\"label\":\"" << item.label << "\""
        << ",\"kind\":" << static_cast<int>(item.kind);
    if (!item.detail.empty()) {
        oss << ",\"detail\":\"" << item.detail << "\"";
    }
    if (!item.insert_text.empty()) {
        oss << ",\"insertText\":\"" << item.insert_text << "\"";
    }
    oss << "}";
    return oss.str();
}

std::string completion_list_to_json(const CompletionList& list) {
    std::ostringstream oss;
    oss << "{\"isIncomplete\":" << (list.is_incomplete ? "true" : "false")
        << ",\"items\":[";
    for (size_t i = 0; i < list.items.size(); ++i) {
        if (i > 0) oss << ",";
        oss << completion_item_to_json(list.items[i]);
    }
    oss << "]}";
    return oss.str();
}

void CompletionList::filter(std::string_view prefix) {
    if (prefix.empty()) return;
    auto to_lower = [](const char value) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    };
    auto it = std::remove_if(items.begin(), items.end(), [&](const CompletionItem& item) {
        auto label_lower = item.label;
        std::transform(label_lower.begin(), label_lower.end(), label_lower.begin(), to_lower);
        auto prefix_lower = std::string(prefix);
        std::transform(prefix_lower.begin(), prefix_lower.end(), prefix_lower.begin(), to_lower);
        return label_lower.find(prefix_lower) == std::string::npos;
    });
    items.erase(it, items.end());
}

} // namespace gspl::ls
