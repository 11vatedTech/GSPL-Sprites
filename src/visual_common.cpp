#include "gspl_sprites/visual_common.hpp"

namespace gspl::sprites::visual {
namespace {

struct LayerEntry { VisualLayer layer; std::string_view name; std::int32_t order; };
constexpr LayerEntry kLayers[] = {
    {VisualLayer::rear_effects,     "rear_effects",     0},
    {VisualLayer::rear_appendages,  "rear_appendages",  1},
    {VisualLayer::body,             "body",             2},
    {VisualLayer::front_appendages, "front_appendages", 3},
    {VisualLayer::facial,           "facial",           4},
    {VisualLayer::markings,         "markings",         5},
    {VisualLayer::shadows,          "shadows",          6},
    {VisualLayer::highlights,       "highlights",       7},
    {VisualLayer::front_effects,    "front_effects",    8},
    {VisualLayer::emission,         "emission",         9},
    {VisualLayer::outline,          "outline",          10},
};

struct RoleEntry { ColorRole role; std::string_view name; };
constexpr RoleEntry kRoles[] = {
    {ColorRole::primary,   "primary"},
    {ColorRole::secondary, "secondary"},
    {ColorRole::accent,    "accent"},
    {ColorRole::eye,       "eye"},
    {ColorRole::emission,  "emission"},
    {ColorRole::shadow,    "shadow"},
    {ColorRole::highlight, "highlight"},
    {ColorRole::outline,   "outline"},
    {ColorRole::effect,    "effect"},
    {ColorRole::warning,   "warning"},
    {ColorRole::damage,    "damage"},
    {ColorRole::custom,    "custom"},
};

} // namespace

std::string_view layer_name(VisualLayer layer) noexcept {
  for (const auto& e : kLayers) {
    if (e.layer == layer) return e.name;
  }
  return "unknown";
}

std::optional<VisualLayer> layer_from_name(std::string_view name) noexcept {
  for (const auto& e : kLayers) {
    if (e.name == name) return e.layer;
  }
  return std::nullopt;
}

std::int32_t layer_order(VisualLayer layer) noexcept {
  for (const auto& e : kLayers) {
    if (e.layer == layer) return e.order;
  }
  return 100;
}

std::string_view color_role_name(ColorRole role) noexcept {
  for (const auto& e : kRoles) {
    if (e.role == role) return e.name;
  }
  return "custom";
}

std::optional<ColorRole> color_role_from_name(std::string_view name) noexcept {
  for (const auto& e : kRoles) {
    if (e.name == name) return e.role;
  }
  return std::nullopt;
}

} // namespace gspl::sprites::visual
