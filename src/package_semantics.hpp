#pragma once

#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <string_view>

namespace gspl::sprites {

[[nodiscard]] ValidationResult
validate_package_semantic_closure(std::string_view asset_graph,
                                  std::string_view provenance,
                                  std::uint32_t maximum_records);

} // namespace gspl::sprites
