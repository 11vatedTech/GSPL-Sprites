#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/style.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── StyleProgram ──
 * Style as typed, factorized visual behavior (not a label). Line,
 * Shading, Motion, Palette, FX and Compositing programs compose with
 * explicit precedence (base < project < entity < form < performance) and
 * lower deterministically to the existing StyleSemantics consumed by the
 * Visual Semantic Compiler. Labels exist only as presets.
 * See docs/architecture/STYLE_PROGRAM_ARCHITECTURE.md. */

struct LineProgram {
  double line_weight{0.0};
  double line_taper{0.0};
  OutlineSelection outline_selection{OutlineSelection::silhouette};
  double corner_sharpness{0.5};
  double edge_softness{0.0};
};

struct ShadingProgram {
  ShadowModel shadow_model{ShadowModel::gradient};
  std::uint32_t band_count{1};      // cel bands (1 = single terminator)
  double ramp_curve{1.0};           // ramp shape (1 = linear)
  HighlightModel highlight_model{HighlightModel::soft};
  double hatch_density{0.0};        // 0 = none
  double halftone_scale{0.0};       // 0 = none
  double terminator_anchor{0.5};    // shadow terminator position policy
};

struct MotionProgram {
  double timing_cadence{1.0};
  double spacing_behavior{0.5};     // 0 = even, 1 = snappy ease
  double smear_threshold{0.0};      // velocity magnitude gating smears
  double stepped_cadence{0.0};      // 0 = continuous, 1 = 12fps feel
  double motion_abstraction{0.0};   // speed-line/ghost strength
};

struct PaletteProgram {
  PalettePolicy palette_policy{PalettePolicy::preserve};
  double value_grouping{0.0};
  double saturation_behavior{1.0};
  double hue_shift_curve{0.0};      // ramp hue-shift strength
  std::uint32_t max_colors{0};      // 0 = unlimited
};

struct FxProgram {
  double aura_interpretation{1.0};
  double emission_interpretation{1.0};
  double halftone_dots{0.0};
  double registration_shift{0.0};
  double impact_frames{0.0};        // 0 = none, 1 = single-frame abstracts
  double speed_lines{0.0};
};

struct CompositingProgram {
  LayerCompositing layer_compositing{LayerCompositing::source_over};
  double alpha_policy{1.0};
  AntiAliasPolicy aa_policy{AntiAliasPolicy::analytic};
  double pixel_quantization{0.0};
};

struct StyleProgram {
  std::string schema{"gspl.style-program/0.1"};
  std::string name;                 // preset name or ""
  LineProgram line;
  ShadingProgram shading;
  MotionProgram motion;
  PaletteProgram palette;
  FxProgram fx;
  CompositingProgram compositing;
  std::vector<StylePatch> patches;  // extra precedence-ordered patches
};

/* Presets mirror make_style_preset behaviors (clean-flat, inked,
 * soft-shaded, pixel-constrained). Unknown names return a neutral
 * program. */
[[nodiscard]] StyleProgram make_style_program_preset(std::string_view name);

/* Deterministic lowering to the existing StyleSemantics. */
[[nodiscard]] StyleSemantics effective_style(const StyleProgram& program);

[[nodiscard]] ValidationResult validate_style_program(const StyleProgram& program);
[[nodiscard]] std::string canonicalize_style_program(const StyleProgram& program);
[[nodiscard]] std::string style_program_identity(const StyleProgram& program);

} // namespace gspl::sprites::visual
