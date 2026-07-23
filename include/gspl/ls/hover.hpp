#pragma once

#include "gspl/ls/navigation.hpp"
#include "gspl/ls/diagnostic.hpp"
#include <string>
#include <string_view>

namespace gspl::ls {

HoverInfo get_hover_info(std::string_view source, std::string_view uri,
                         std::uint32_t line, std::uint32_t column);

std::string hover_info_to_json(const HoverInfo& info);

} // namespace gspl::ls
