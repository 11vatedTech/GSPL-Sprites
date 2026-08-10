#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Marking / pattern semantics ──
 * A marking binds to a semantic surface (part_id) and is drawn in that
 * part's local frame, masked by the part's own coverage. Markings are
 * geometry + color semantics, never baked pixel data. */

enum class MarkingKind {
  stripe,
  spot,
  band,
  gradient_region,
  symbol,
  vein,
  circuit,
  scar,
  tattoo,
  fur_marking,
  electrical,
  procedural
};

[[nodiscard]] std::string_view marking_kind_name(MarkingKind kind) noexcept;
[[nodiscard]] std::optional<MarkingKind> marking_kind_from_name(std::string_view name) noexcept;

struct Marking {
  std::string id;
  MarkingKind kind{MarkingKind::stripe};
  std::string part_id;            // semantic surface binding (must exist in morphology)
  std::uint32_t color{0xFFFFFFFF}; // 0xRRGGBBAA packed
  ColorRole color_role{ColorRole::custom};
  double opacity{1.0};
  double x{};
  double y{};
  double rotation_degrees{};
  double scale{1.0};
  std::vector<visual::Vec2> path;    // stripe/band/vein/circuit/electrical/scar/tattoo/symbol
  std::vector<double> widths;        // per-vertex stroke widths (taper)
  std::vector<visual::Vec2> spots;   // spot centers (local space)
  double spot_radius{1.0};
  double band_width{1.0};
  std::uint64_t seed{0};
  double density{0.1};
};

[[nodiscard]] ValidationResult validate_markings(std::span<const Marking> markings,
                                                 std::span<const std::string> part_ids,
                                                 std::uint32_t max_markings = 128);
[[nodiscard]] std::string canonicalize_marking(const Marking& marking);
[[nodiscard]] std::string canonicalize_markings(std::span<const Marking> markings);

} // namespace gspl::sprites::visual
