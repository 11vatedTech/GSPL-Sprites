#pragma once

#include "gspl_sprites/core.hpp"

#include <optional>
#include <string_view>

namespace gspl::sprites {

/* RightsClass <-> text mapping authority. All platform conversion sites
 * (seed parser, authoring, authoring_io, lowering, package serialization)
 * must use these functions; no other rights name table may exist. */

[[nodiscard]] std::string_view rights_class_to_string(RightsClass classification) noexcept;

[[nodiscard]] std::optional<RightsClass>
rights_class_from_string(std::string_view text) noexcept;

[[nodiscard]] RightsClass
lower_rights_class(std::string_view classification) noexcept;

} // namespace gspl::sprites
