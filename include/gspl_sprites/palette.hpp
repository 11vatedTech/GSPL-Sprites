#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_common.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace gspl::sprites::visual {

/* ── Color / palette semantics ──
 * Single color authority for the Native Visual Core. Colors are stored
 * packed as 0xRRGGBBAA. A palette maps semantic color roles to concrete
 * colors; explicit entity colors remain compatible via the 'custom' role. */

[[nodiscard]] std::optional<std::uint32_t> parse_hex_color(std::string_view hex);  // "#rrggbb"
[[nodiscard]] std::string rgba32_to_hex(std::uint32_t rgba);
[[nodiscard]] std::uint32_t mix_colors(std::uint32_t a, std::uint32_t b, double t);
[[nodiscard]] std::uint32_t scale_rgb(std::uint32_t rgba, double factor);
[[nodiscard]] std::uint32_t quantize_color(std::uint32_t rgba, std::uint32_t levels);
[[nodiscard]] double color_luminance(std::uint32_t rgba);
[[nodiscard]] std::uint32_t with_alpha_value(std::uint32_t rgba, std::uint8_t alpha);

struct PaletteDefinition {
  std::string schema{"gspl.palette/0.1"};
  std::map<std::string, std::uint32_t, std::less<>> colors;  // role name -> 0xRRGGBBAA
  double minimum_separation{0.0};
  std::uint32_t max_palette_size{0};
  std::string form_id;
};

[[nodiscard]] std::uint32_t palette_color(const PaletteDefinition& palette, ColorRole role,
                                          std::uint32_t fallback = 0xFFFFFFFF);
[[nodiscard]] std::optional<std::uint32_t> palette_color_by_name(const PaletteDefinition& palette,
                                                                 std::string_view role_name);
[[nodiscard]] PaletteDefinition make_default_palette(std::string_view primary_hex, std::string_view accent_hex,
                                                     std::string_view emissive_hex, std::string_view aura_hex);
[[nodiscard]] PaletteDefinition make_transformed_palette(std::string_view primary_hex, std::string_view accent_hex,
                                                         std::string_view emissive_hex, std::string_view aura_hex);
[[nodiscard]] ValidationResult validate_palette(const PaletteDefinition& palette);
[[nodiscard]] std::string canonicalize_palette(const PaletteDefinition& palette);
[[nodiscard]] std::string palette_identity(const PaletteDefinition& palette);

struct VisualPart;

// Resolve a part's concrete color: explicit literal color wins, else role color.
[[nodiscard]] std::uint32_t resolve_part_color(const VisualPart& part, const PaletteDefinition& palette);

// Quantize RGB channels to a deterministic N-level grid (pixel-art style policy).
[[nodiscard]] std::uint32_t quantize_color(std::uint32_t rgba, double levels);

} // namespace gspl::sprites::visual
