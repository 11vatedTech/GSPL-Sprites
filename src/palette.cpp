#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/visual_morphology.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

[[nodiscard]] int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

[[nodiscard]] std::uint8_t clamp_byte(double v) {
  const int i = static_cast<int>(std::lround(v));
  return static_cast<std::uint8_t>(std::clamp(i, 0, 255));
}

} // namespace

std::optional<std::uint32_t> parse_hex_color(std::string_view hex) {
  if (hex.size() != 7 || hex[0] != '#') return std::nullopt;
  std::uint32_t rgb = 0;
  for (std::size_t i = 1; i < 7; ++i) {
    const int d = hex_digit(hex[i]);
    if (d < 0) return std::nullopt;
    rgb = (rgb << 4) | static_cast<std::uint32_t>(d);
  }
  return (rgb << 8) | 0xFFu;  // 0xRRGGBBAA, A = 0xFF
}

std::string rgba32_to_hex(std::uint32_t rgba) {
  char buf[8] = {'#', '0', '0', '0', '0', '0', '0', '\0'};
  const char* hex = "0123456789abcdef";
  for (int i = 0; i < 3; ++i) {
    const int shift = 24 - i * 8;
    buf[1 + i * 2] = hex[(rgba >> (shift + 4)) & 0xF];
    buf[2 + i * 2] = hex[(rgba >> shift) & 0xF];
  }
  return std::string(buf, 7);
}

std::uint32_t mix_colors(std::uint32_t a, std::uint32_t b, double t) {
  t = std::clamp(t, 0.0, 1.0);
  const double r = (a >> 24 & 0xFF) * (1.0 - t) + (b >> 24 & 0xFF) * t;
  const double g = (a >> 16 & 0xFF) * (1.0 - t) + (b >> 16 & 0xFF) * t;
  const double bl = (a >> 8 & 0xFF) * (1.0 - t) + (b >> 8 & 0xFF) * t;
  const double al = (a & 0xFF) * (1.0 - t) + (b & 0xFF) * t;
  return (static_cast<std::uint32_t>(clamp_byte(r)) << 24) |
         (static_cast<std::uint32_t>(clamp_byte(g)) << 16) |
         (static_cast<std::uint32_t>(clamp_byte(bl)) << 8) |
         static_cast<std::uint32_t>(clamp_byte(al));
}

std::uint32_t scale_rgb(std::uint32_t rgba, double factor) {
  const double r = (rgba >> 24 & 0xFF) * factor;
  const double g = (rgba >> 16 & 0xFF) * factor;
  const double b = (rgba >> 8 & 0xFF) * factor;
  return (static_cast<std::uint32_t>(clamp_byte(r)) << 24) |
         (static_cast<std::uint32_t>(clamp_byte(g)) << 16) |
         (static_cast<std::uint32_t>(clamp_byte(b)) << 8) |
         (rgba & 0xFF);
}

std::uint32_t quantize_color(std::uint32_t rgba, std::uint32_t levels) {
  if (levels < 2) return rgba;
  auto quantize = [&](std::uint32_t v) -> std::uint32_t {
    const double scaled = static_cast<double>(v) / 255.0 * static_cast<double>(levels - 1);
    return static_cast<std::uint32_t>(std::lround(scaled)) * 255u / (levels - 1);
  };
  return (quantize(rgba >> 24 & 0xFF) << 24) |
         (quantize(rgba >> 16 & 0xFF) << 16) |
         (quantize(rgba >> 8 & 0xFF) << 8) |
         (rgba & 0xFF);
}

double color_luminance(std::uint32_t rgba) {
  const double r = static_cast<double>(rgba >> 24 & 0xFF) / 255.0;
  const double g = static_cast<double>(rgba >> 16 & 0xFF) / 255.0;
  const double b = static_cast<double>(rgba >> 8 & 0xFF) / 255.0;
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

std::uint32_t with_alpha_value(std::uint32_t rgba, std::uint8_t alpha) {
  return (rgba & 0xFFFFFF00u) | alpha;
}

std::uint32_t palette_color(const PaletteDefinition& palette, ColorRole role, std::uint32_t fallback) {
  const auto it = palette.colors.find(std::string(color_role_name(role)));
  return (it != palette.colors.end()) ? it->second : fallback;
}

std::optional<std::uint32_t> palette_color_by_name(const PaletteDefinition& palette, std::string_view role_name) {
  const auto it = palette.colors.find(std::string(role_name));
  if (it == palette.colors.end()) return std::nullopt;
  return it->second;
}

PaletteDefinition make_default_palette(std::string_view primary_hex, std::string_view accent_hex,
                                       std::string_view emissive_hex, std::string_view aura_hex) {
  PaletteDefinition p;
  const std::uint32_t primary = parse_hex_color(primary_hex).value_or(0x3A3350FF);
  const std::uint32_t accent = parse_hex_color(accent_hex).value_or(0x56F1FFFF);
  const std::uint32_t emission = parse_hex_color(emissive_hex).value_or(accent);
  const std::uint32_t aura = parse_hex_color(aura_hex).value_or(accent);
  p.colors["primary"] = primary;
  p.colors["secondary"] = scale_rgb(primary, 0.6);
  p.colors["accent"] = accent;
  p.colors["eye"] = mix_colors(accent, 0xFFFFFFu, 0.35);
  p.colors["emission"] = emission;
  p.colors["shadow"] = scale_rgb(primary, 0.32);
  p.colors["highlight"] = mix_colors(primary, 0xFFFFFFu, 0.32);
  p.colors["outline"] = 0x14101BFF;
  p.colors["effect"] = aura;
  p.colors["warning"] = 0xE07A1FFF;
  p.colors["damage"] = 0xC6283FFF;
  return p;
}

PaletteDefinition make_transformed_palette(std::string_view primary_hex, std::string_view accent_hex,
                                           std::string_view emissive_hex, std::string_view aura_hex) {
  PaletteDefinition p;
  const std::uint32_t primary = parse_hex_color(primary_hex).value_or(0x241A33FF);
  const std::uint32_t accent = parse_hex_color(accent_hex).value_or(0xFFD700FF);
  const std::uint32_t emission = parse_hex_color(emissive_hex).value_or(accent);
  const std::uint32_t aura = parse_hex_color(aura_hex).value_or(accent);
  p.colors["primary"] = primary;
  p.colors["secondary"] = scale_rgb(primary, 0.55);
  p.colors["accent"] = accent;
  p.colors["eye"] = mix_colors(accent, 0xFFFFFFu, 0.45);
  p.colors["emission"] = emission;
  p.colors["shadow"] = scale_rgb(primary, 0.3);
  p.colors["highlight"] = mix_colors(primary, 0xFFFFFFu, 0.4);
  p.colors["outline"] = 0x0A070FFF;
  p.colors["effect"] = aura;
  p.colors["warning"] = 0xFF9A3DFF;
  p.colors["damage"] = 0xFF4D5EFF;
  return p;
}

ValidationResult validate_palette(const PaletteDefinition& palette) {
  ValidationResult result;
  if (palette.schema != "gspl.palette/0.1") {
    result.diagnostics.push_back({"PALETTE_SCHEMA", "unexpected palette schema '" + palette.schema + "'"});
  }
  if (palette.max_palette_size > 0 && palette.colors.size() > palette.max_palette_size) {
    result.diagnostics.push_back({"PALETTE_SIZE_EXCEEDED",
        "palette size " + std::to_string(palette.colors.size()) + " exceeds max " + std::to_string(palette.max_palette_size)});
  }
  if (palette.minimum_separation < 0.0) {
    result.diagnostics.push_back({"PALETTE_SEPARATION", "minimum_separation must be >= 0"});
  }
  for (const auto& [role, color] : palette.colors) {
    (void)color;
    if (!color_role_from_name(role) && role != "custom") {
      result.diagnostics.push_back({"PALETTE_UNKNOWN_ROLE", "unknown palette role '" + role + "'"});
    }
  }
  return result;
}

std::string canonicalize_palette(const PaletteDefinition& palette) {
  std::ostringstream out;
  out << "{\"schema\":\"" << palette.schema << "\",\"form\":\"" << palette.form_id << "\"";
  out << ",\"min_sep\":" << palette.minimum_separation
      << ",\"max_size\":" << palette.max_palette_size;
  out << ",\"colors\":[";
  bool first = true;
  for (const auto& [role, color] : palette.colors) {
    if (!first) out << ",";
    first = false;
    out << "{\"role\":\"" << role << "\",\"c\":" << color << "}";
  }
  out << "]}";
  return out.str();
}

std::string palette_identity(const PaletteDefinition& palette) {
  return gspl::sprites::sha256(canonicalize_palette(palette));
}


std::uint32_t resolve_part_color(const VisualPart& part, const PaletteDefinition& palette) {
  if (!part.explicit_color.empty()) {
    const auto parsed = parse_hex_color(part.explicit_color);
    if (parsed) return *parsed;
  }
  return palette_color(palette, part.color_role);
}

std::uint32_t quantize_color(std::uint32_t rgba, double levels) {
  const double lv = std::max(2.0, levels);
  auto q8 = [lv](std::uint8_t v) {
    const double step = std::clamp(static_cast<double>(std::lround(static_cast<double>(v) / 255.0 * (lv - 1.0))), 0.0, lv - 1.0);
    return static_cast<std::uint8_t>(std::lround(step * 255.0 / (lv - 1.0)));
  };
  const std::uint32_t r = static_cast<std::uint32_t>(q8(static_cast<std::uint8_t>(rgba >> 24)));
  const std::uint32_t g = static_cast<std::uint32_t>(q8(static_cast<std::uint8_t>(rgba >> 16)));
  const std::uint32_t b = static_cast<std::uint32_t>(q8(static_cast<std::uint8_t>(rgba >> 8)));
  return (r << 24) | (g << 16) | (b << 8) | 0xFFu;
}

} // namespace gspl::sprites::visual
