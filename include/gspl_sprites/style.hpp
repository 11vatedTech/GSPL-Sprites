#pragma once

#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Style semantics ──
 * Style is a deterministic description of VISUAL BEHAVIOR (contour,
 * line, palette policy, shadow model...), not a string label. Labels
 * exist only as presets (make_style_preset). Style is composable with
 * explicit precedence: base < project < entity < form < performance. */

enum class ContourBehavior { smooth, angular, chunky };
enum class OutlineSelection { none, silhouette, all_parts };
enum class PalettePolicy { preserve, harmonize, quantize };
enum class ShadowModel { flat, gradient, cell };
enum class HighlightModel { none, soft, strong };
enum class AntiAliasPolicy { none, analytic, supersample };
enum class LayerCompositing { source_over, additive };

struct StyleSemantics {
  std::string schema{"gspl.style/0.1"};
  double shape_abstraction{0.35};
  double proportion_exaggeration{0.0};
  ContourBehavior contour{ContourBehavior::smooth};
  double line_weight{0.0};
  double line_taper{0.0};
  OutlineSelection outline_selection{OutlineSelection::silhouette};
  double corner_sharpness{0.5};
  double curve_smoothness{0.75};
  PalettePolicy palette_policy{PalettePolicy::preserve};
  double value_grouping{0.0};
  double saturation_behavior{1.0};
  ShadowModel shadow_model{ShadowModel::gradient};
  HighlightModel highlight_model{HighlightModel::soft};
  double texture_density{0.0};
  double detail_density{1.0};
  double edge_softness{0.0};
  AntiAliasPolicy aa_policy{AntiAliasPolicy::analytic};
  double pixel_quantization{0.0};
  LayerCompositing layer_compositing{LayerCompositing::source_over};
  // StyleProgram-driven fields (lowered from factorized program):
  std::uint32_t band_count{1};   // cel shading bands (1 = smooth)
  std::uint32_t max_colors{0};   // palette limit (0 = unlimited)
};

struct StylePatch {
  std::string scope;  // "project" | "entity" | "form" | "performance"
  std::optional<double> shape_abstraction;
  std::optional<double> proportion_exaggeration;
  std::optional<ContourBehavior> contour;
  std::optional<double> line_weight;
  std::optional<double> line_taper;
  std::optional<OutlineSelection> outline_selection;
  std::optional<double> corner_sharpness;
  std::optional<double> curve_smoothness;
  std::optional<PalettePolicy> palette_policy;
  std::optional<double> value_grouping;
  std::optional<double> saturation_behavior;
  std::optional<ShadowModel> shadow_model;
  std::optional<HighlightModel> highlight_model;
  std::optional<double> texture_density;
  std::optional<double> detail_density;
  std::optional<double> edge_softness;
  std::optional<AntiAliasPolicy> aa_policy;
  std::optional<double> pixel_quantization;
  std::optional<LayerCompositing> layer_compositing;
};

[[nodiscard]] StyleSemantics make_style_preset(std::string_view name);
[[nodiscard]] StyleSemantics compose_style(const StyleSemantics& base, std::span<const StylePatch> patches);
[[nodiscard]] ValidationResult validate_style(const StyleSemantics& style);
[[nodiscard]] std::string canonicalize_style(const StyleSemantics& style);
[[nodiscard]] std::string canonicalize_style_patch(const StylePatch& patch);
[[nodiscard]] std::string style_identity(const StyleSemantics& style);

} // namespace gspl::sprites::visual
