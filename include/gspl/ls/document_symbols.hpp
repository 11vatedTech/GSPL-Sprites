#pragma once

#include "gspl/ls/navigation.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace gspl::ls {

class DocumentSymbolBuilder {
public:
    DocumentSymbolBuilder() = default;
    
    std::vector<SymbolInfo> build(std::string_view source, std::string_view uri);
    SymbolInfo build_single(std::string_view source, std::string_view uri, std::string_view name);
    
private:
    SymbolInfo make_symbol(std::string_view name, std::string_view kind,
                          uint32_t start_line, uint32_t start_col,
                          uint32_t end_line, uint32_t end_col);
};

} // namespace gspl::ls
