#pragma once

#include "diagnostic.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace gspl::ls {

struct Location {
    std::string uri;
    Range range;
};

struct SymbolInfo {
    std::string name;
    std::string kind;
    Range range;
    Range selection_range;
    std::vector<SymbolInfo> children;
};

struct Reference {
    std::string uri;
    Range range;
    std::string context;
};

struct HoverInfo {
    std::string contents;
    Range range;
};

std::string symbol_info_to_json(const SymbolInfo& sym);
std::string location_to_json(const Location& loc);
} // namespace gspl::ls
