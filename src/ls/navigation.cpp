#include "gspl/ls/navigation.hpp"
#include <sstream>

namespace gspl::ls {

std::string location_to_json(const Location& loc) {
    std::ostringstream oss;
    oss << "{\"uri\":\"" << loc.uri << "\""
        << ",\"range\":{"
        << "\"start\":{\"line\":" << loc.range.start.line
        << ",\"character\":" << loc.range.start.column << "},"
        << "\"end\":{\"line\":" << loc.range.end.line
        << ",\"character\":" << loc.range.end.column << "}"
        << "}}";
    return oss.str();
}

std::string symbol_info_to_json(const SymbolInfo& sym) {
    std::ostringstream oss;
    oss << "{\"name\":\"" << sym.name << "\""
        << ",\"kind\":\"" << sym.kind << "\""
        << ",\"range\":{"
        << "\"start\":{\"line\":" << sym.range.start.line
        << ",\"character\":" << sym.range.start.column << "},"
        << "\"end\":{\"line\":" << sym.range.end.line
        << ",\"character\":" << sym.range.end.column << "}"
        << "}"
        << ",\"selectionRange\":{"
        << "\"start\":{\"line\":" << sym.selection_range.start.line
        << ",\"character\":" << sym.selection_range.start.column << "},"
        << "\"end\":{\"line\":" << sym.selection_range.end.line
        << ",\"character\":" << sym.selection_range.end.column << "}"
        << "}";
    if (!sym.children.empty()) {
        oss << ",\"children\":[";
        for (size_t i = 0; i < sym.children.size(); ++i) {
            if (i > 0) oss << ",";
            oss << symbol_info_to_json(sym.children[i]);
        }
        oss << "]";
    }
    oss << "}";
    return oss.str();
}

std::string reference_to_json(const Reference& ref) {
    std::ostringstream oss;
    oss << "{\"uri\":\"" << ref.uri << "\""
        << ",\"range\":{"
        << "\"start\":{\"line\":" << ref.range.start.line
        << ",\"character\":" << ref.range.start.column << "},"
        << "\"end\":{\"line\":" << ref.range.end.line
        << ",\"character\":" << ref.range.end.column << "}"
        << "}}";
    return oss.str();
}

std::string hover_info_to_json(const HoverInfo& hover) {
    std::ostringstream oss;
    oss << "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" << hover.contents << "\"}";
    if (hover.range.start.line != 0 || hover.range.end.line != 0) {
        oss << ",\"range\":{"
            << "\"start\":{\"line\":" << hover.range.start.line
            << ",\"character\":" << hover.range.start.column << "},"
            << "\"end\":{\"line\":" << hover.range.end.line
            << ",\"character\":" << hover.range.end.column << "}"
            << "}";
    }
    oss << "}";
    return oss.str();
}

} // namespace gspl::ls
