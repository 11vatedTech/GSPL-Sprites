#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/style.hpp"
#include "gspl_sprites/visual_common.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace gspl::sprites::visual {

/* ── Material semantics ──
 * Encodes WHAT A SURFACE VISUALLY IS (fur, metal, glass, energy...) as
 * data, independently of any renderer. Not a PBR model; a deterministic
 * surface-response vocabulary. Class names are data (unknown classes
 * degrade to a neutral default deterministically). */

struct MaterialSemantics {
  std::string schema{"gspl.material/0.1"};
  std::string id;
  std::string class_name{"skin"};
  ColorRole base_color_role{ColorRole::primary};
  double roughness{0.5};
  double reflectivity{0.0};
  double metallicity{0.0};
  double translucency{0.0};
  double emission{0.0};
  double surface_texture_scale{1.0};
  double anisotropy{0.0};
  double edge_response{0.5};
  double rim_response{0.0};
  double subsurface_intent{0.0};
};

[[nodiscard]] MaterialSemantics make_material(std::string_view class_name, std::string id = {});
[[nodiscard]] MaterialSemantics resolve_material(const MaterialSemantics& base, const StyleSemantics& style);
[[nodiscard]] ValidationResult validate_material(const MaterialSemantics& material);
[[nodiscard]] std::string canonicalize_material(const MaterialSemantics& material);

} // namespace gspl::sprites::visual
