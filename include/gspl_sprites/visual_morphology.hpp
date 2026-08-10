#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/markings.hpp"
#include "gspl_sprites/visual_common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Visual Morphology v2 ──
 * The evolved, renderer-agnostic visual structure. It is a DERIVED
 * semantic manifestation description (never an entity truth): it exists
 * to describe what should be visible for a given form/style/pose.
 *
 * Shape kinds are general (ellipse, capsule, rounded rect, polygon,
 * ring, open/closed path) and semantic roles are extensible strings -
 * no Voltfox-specific anatomy is encoded here.
 */

enum class VisualShapeKind {
  ellipse,
  capsule,
  rounded_rect,
  polygon,
  ring,
  open_path,
  closed_path
};

[[nodiscard]] std::string_view visual_shape_kind_name(VisualShapeKind kind) noexcept;
[[nodiscard]] std::optional<VisualShapeKind> visual_shape_kind_from_name(std::string_view name) noexcept;

struct VisualPart {
  std::string id;
  std::string parent;                    // hierarchical attachment ("" = root)
  double x{};
  double y{};
  double z{};                            // depth coordinate (2.5D/3D capability)
  double size_x{1.0};
  double size_y{1.0};
  double size_z{1.0};
  double rotation_degrees{};
  VisualShapeKind shape{VisualShapeKind::ellipse};
  std::string explicit_color;            // compatible literal color "#rrggbb" or empty
  ColorRole color_role{ColorRole::primary};
  std::string material_class;            // data-driven surface class: fur/skin/metal/...
  std::string material_id;               // explicit material id ("" = auto from class)
  std::string semantic_role;             // extensible: body-mass, limb, eye, aura, ...
  std::string bone_id;                   // rig binding
  std::string socket_id;                 // attachment anchor
  VisualLayer layer{VisualLayer::body};
  std::int32_t z_order{};                // fine ordering within layer
  double opacity{1.0};
  bool emissive{};
  bool visible{true};
  bool silhouette_contribution{true};
  std::string projection_behavior{"default"};   // future: billboard/screen/ground
  std::string render_group;              // optional grouping
  std::vector<std::string> marking_ids;  // markings bound to this surface
  std::vector<visual::Vec2> polygon;     // polygon shape vertices (local space)
  visual::Path path;                     // open/closed path shape (local space)
  std::vector<double> stroke_widths;     // per-vertex stroke widths (taper)
  double ring_inner_scale{0.7};          // ring inner radius = outer * scale
};

struct VisualMorphologyV2 {
  std::string schema{"gspl.visual-morphology/0.1"};
  std::string form_id;
  std::map<std::string, VisualPart, std::less<>> parts;
  std::vector<Marking> markings;
  std::vector<std::string> layer_order;  // deterministic compositing order
};

[[nodiscard]] ValidationResult validate_visual_morphology(const VisualMorphologyV2& morph,
                                                          std::uint32_t max_parts = 256);
[[nodiscard]] std::string canonicalize_visual_morphology(const VisualMorphologyV2& morph);
[[nodiscard]] std::string visual_morphology_identity(const VisualMorphologyV2& morph);

/* ── v1 -> v2 deterministic lowering (compatibility authority) ──
 * The sealed v1 morphology map is preserved as schema-compatible v1;
 * this function produces the v2 description used by the Visual Semantic
 * Compiler. No semantic reinterpretation: all v1 fields flow through.
 */
[[nodiscard]] VisualMorphologyV2 lower_visual_morphology_v1(
    const std::map<std::string, MorphologyPart, std::less<>>& v1,
    std::string_view form_id);

} // namespace gspl::sprites::visual
