#pragma once

#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/fx_semantics.hpp"
#include "gspl_sprites/material.hpp"
#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/style.hpp"
#include "gspl_sprites/visual_morphology.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Canonical Visual IR ──
 * The resolved, validated manifestation description produced by the
 * Visual Semantic Compiler. It carries EVERYTHING the renderer needs;
 * renderer backends never access CanonicalEntity directly and never
 * reinterpret semantics. It is a derived description (Visual
 * Manifestation Identity taxonomy), never entity truth. */

struct ProjectionSpec {
  std::string kind{"2d"};     // future: 2.5d, 3d
  std::uint32_t width{128};
  std::uint32_t height{128};
  bool mirror_x{};            // facing left
};

struct LightingSpec {
  double light_degrees{45.0}; // direction light comes FROM (0 = +x, 90 = +y)
  double ambient{0.42};
  double key{0.55};
  double fill{0.18};
  double rim{0.15};
  bool drop_shadow{true};
  double shadow_offset_x{2.0};
  double shadow_offset_y{3.0};
  double shadow_alpha{0.35};
};

struct ChannelRequest {
  ChannelMapKind kind{ChannelMapKind::emissive};
  std::string id;
};

struct VisualLimits {
  std::uint32_t max_parts{256};
  std::uint32_t max_materials{256};
  std::uint32_t max_markings{128};
  std::uint32_t max_canvas_width{2048};
  std::uint32_t max_canvas_height{2048};
  std::uint32_t max_channel_requests{16};
  // Semantic LOD: when non-zero, features with min_resolution above
  // this target are discarded (see VisualFeature::min_resolution).
  std::uint32_t target_resolution{0};
};

struct VisualIr {
  std::string schema{"gspl.visual-ir/0.1"};
  std::string entity_identity;      // visual manifestation identity
  std::string form_id;
  ProjectionSpec projection;
  LightingSpec lighting;
  VisualMorphologyV2 morphology;
  StyleSemantics style;
  PaletteDefinition palette;
  std::vector<MaterialSemantics> materials;
  PerformanceState performance;
  std::vector<std::string> layer_order;
  std::vector<ChannelRequest> channel_requests;
  // Temporal identity: part_id -> deterministic temporal label (stable
  // hash of the canon structure/parent/role). Persists across frames for
  // contour/landmark/marking correspondence.
  std::map<std::string, std::string, std::less<>> temporal_part_labels;
  // FX state: resolved from performance events (impact, emission, aura).
  // The renderer consults fx_state for active effect draw params.
  FxState fx_state;
};

[[nodiscard]] ValidationResult validate_visual_ir(const VisualIr& ir, const VisualLimits& limits = {});
[[nodiscard]] std::string canonicalize_visual_ir(const VisualIr& ir);
[[nodiscard]] std::string visual_ir_identity(const VisualIr& ir);

} // namespace gspl::sprites::visual
