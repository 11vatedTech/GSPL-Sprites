#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace gspl::sprites::visual {

/* ── Shared visual semantics enums ──
 * Deterministic compositing order for visual layers and the canonical
 * color-role vocabulary. These are the only layer/role authorities in
 * the Native Visual Core. */

enum class VisualLayer {
  rear_effects,
  rear_appendages,
  body,
  front_appendages,
  facial,
  markings,
  shadows,
  highlights,
  front_effects,
  emission,
  outline
};

[[nodiscard]] std::string_view layer_name(VisualLayer layer) noexcept;
[[nodiscard]] std::optional<VisualLayer> layer_from_name(std::string_view name) noexcept;
[[nodiscard]] std::int32_t layer_order(VisualLayer layer) noexcept;

enum class ColorRole {
  primary,
  secondary,
  accent,
  eye,
  emission,
  shadow,
  highlight,
  outline,
  effect,
  warning,
  damage,
  custom
};

[[nodiscard]] std::string_view color_role_name(ColorRole role) noexcept;
[[nodiscard]] std::optional<ColorRole> color_role_from_name(std::string_view name) noexcept;

} // namespace gspl::sprites::visual
