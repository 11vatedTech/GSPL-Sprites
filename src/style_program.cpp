#include "gspl_sprites/core.hpp"
#include "gspl_sprites/style_program.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace gspl::sprites::visual {
namespace {

[[nodiscard]] std::string d2s(double v) {
  char buf[32];
  const auto res = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::general, 8);
  return std::string(buf, res.ptr);
}

void add_diag(ValidationResult& r, std::string code, std::string msg,
              DiagnosticSeverity severity = DiagnosticSeverity::error) {
  r.diagnostics.push_back({std::move(code), std::move(msg), severity});
}

[[nodiscard]] std::string outline_name(OutlineSelection v) {
  switch (v) {
    case OutlineSelection::none: return "none";
    case OutlineSelection::silhouette: return "silhouette";
    case OutlineSelection::all_parts: return "all_parts";
  }
  return "none";
}

[[nodiscard]] std::string shadow_name(ShadowModel v) {
  switch (v) {
    case ShadowModel::flat: return "flat";
    case ShadowModel::gradient: return "gradient";
    case ShadowModel::cell: return "cell";
  }
  return "flat";
}

[[nodiscard]] std::string highlight_name(HighlightModel v) {
  switch (v) {
    case HighlightModel::none: return "none";
    case HighlightModel::soft: return "soft";
    case HighlightModel::strong: return "strong";
  }
  return "none";
}

[[nodiscard]] std::string palette_policy_name(PalettePolicy v) {
  switch (v) {
    case PalettePolicy::preserve: return "preserve";
    case PalettePolicy::harmonize: return "harmonize";
    case PalettePolicy::quantize: return "quantize";
  }
  return "preserve";
}

[[nodiscard]] std::string compositing_name(LayerCompositing v) {
  switch (v) {
    case LayerCompositing::source_over: return "source_over";
    case LayerCompositing::additive: return "additive";
  }
  return "source_over";
}

[[nodiscard]] std::string aa_name(AntiAliasPolicy v) {
  switch (v) {
    case AntiAliasPolicy::none: return "none";
    case AntiAliasPolicy::analytic: return "analytic";
    case AntiAliasPolicy::supersample: return "supersample";
  }
  return "none";
}

} // namespace

StyleProgram make_style_program_preset(std::string_view name) {
  StyleProgram p;
  p.name = std::string(name);
  const StyleSemantics s = make_style_preset(name);
  p.line.line_weight = s.line_weight;
  p.line.line_taper = s.line_taper;
  p.line.outline_selection = s.outline_selection;
  p.line.corner_sharpness = s.corner_sharpness;
  p.line.edge_softness = s.edge_softness;
  p.shading.shadow_model = s.shadow_model;
  p.shading.highlight_model = s.highlight_model;
  p.shading.band_count = (s.shadow_model == ShadowModel::cell) ? 2u : 1u;
  p.shading.ramp_curve = 1.0;
  p.palette.palette_policy = s.palette_policy;
  p.palette.value_grouping = s.value_grouping;
  p.palette.saturation_behavior = s.saturation_behavior;
  p.compositing.aa_policy = s.aa_policy;
  p.compositing.pixel_quantization = s.pixel_quantization;
  p.compositing.layer_compositing = s.layer_compositing;
  return p;
}

StyleSemantics effective_style(const StyleProgram& program) {
  StyleSemantics out;
  const StyleSemantics preset = program.name.empty() ? StyleSemantics{} : make_style_preset(program.name);
  out.shape_abstraction = preset.shape_abstraction;
  out.proportion_exaggeration = preset.proportion_exaggeration;
  out.contour = preset.contour;
  out.curve_smoothness = preset.curve_smoothness;
  out.texture_density = preset.texture_density;
  out.detail_density = preset.detail_density;

  out.line_weight = program.line.line_weight;
  out.line_taper = program.line.line_taper;
  out.outline_selection = program.line.outline_selection;
  out.corner_sharpness = program.line.corner_sharpness;
  out.edge_softness = program.line.edge_softness;

  out.shadow_model = program.shading.shadow_model;
  out.highlight_model = program.shading.highlight_model;
  out.band_count = program.shading.band_count;

  out.palette_policy = program.palette.palette_policy;
  out.value_grouping = program.palette.value_grouping;
  out.saturation_behavior = program.palette.saturation_behavior;
  out.max_colors = program.palette.max_colors;

  out.layer_compositing = program.compositing.layer_compositing;
  out.aa_policy = program.compositing.aa_policy;
  out.pixel_quantization = program.compositing.pixel_quantization;

  // Extra precedence-ordered patches apply last.
  if (!program.patches.empty()) out = compose_style(out, program.patches);
  return out;
}

ValidationResult validate_style_program(const StyleProgram& program) {
  ValidationResult r;
  const bool nonfinite =
      std::isnan(program.line.line_weight) || std::isnan(program.line.line_taper) ||
      std::isnan(program.line.corner_sharpness) || std::isnan(program.line.edge_softness) ||
      std::isnan(program.shading.ramp_curve) || std::isnan(program.shading.hatch_density) ||
      std::isnan(program.shading.halftone_scale) || std::isnan(program.shading.terminator_anchor) ||
      std::isnan(program.motion.timing_cadence) || std::isnan(program.motion.spacing_behavior) ||
      std::isnan(program.motion.smear_threshold) || std::isnan(program.motion.stepped_cadence) ||
      std::isnan(program.motion.motion_abstraction) ||
      std::isnan(program.palette.value_grouping) || std::isnan(program.palette.saturation_behavior) ||
      std::isnan(program.palette.hue_shift_curve) ||
      std::isnan(program.fx.aura_interpretation) || std::isnan(program.fx.emission_interpretation) ||
      std::isnan(program.fx.halftone_dots) || std::isnan(program.fx.registration_shift) ||
      std::isnan(program.fx.impact_frames) || std::isnan(program.fx.speed_lines) ||
      std::isnan(program.compositing.alpha_policy) || std::isnan(program.compositing.pixel_quantization);
  if (nonfinite) add_diag(r, "STYLE_PROGRAM_NONFINITE", "nonfinite style program field");
  if (program.shading.band_count < 1) add_diag(r, "STYLE_PROGRAM_NO_BANDS", "band count must be >= 1");
  if (program.palette.max_colors != 0 && program.palette.max_colors < 2)
    add_diag(r, "STYLE_PROGRAM_BAD_PALETTE_CAP", "palette cap must be 0 or >= 2");
  if (program.compositing.alpha_policy < 0.0 || program.compositing.alpha_policy > 1.0)
    add_diag(r, "STYLE_PROGRAM_BAD_ALPHA", "alpha policy outside [0,1]");
  return r;
}

std::string canonicalize_style_program(const StyleProgram& program) {
  std::string out;
  out += "schema=" + (program.schema.empty() ? std::string("gspl.style-program/0.1") : program.schema) + "\n";
  out += "name=" + program.name + "\n";
  out += "line.line_weight=" + d2s(program.line.line_weight) + "\n";
  out += "line.line_taper=" + d2s(program.line.line_taper) + "\n";
  out += "line.outline=" + outline_name(program.line.outline_selection) + "\n";
  out += "line.corner_sharpness=" + d2s(program.line.corner_sharpness) + "\n";
  out += "line.edge_softness=" + d2s(program.line.edge_softness) + "\n";
  out += "shading.model=" + shadow_name(program.shading.shadow_model) + "\n";
  out += "shading.bands=" + std::to_string(program.shading.band_count) + "\n";
  out += "shading.ramp=" + d2s(program.shading.ramp_curve) + "\n";
  out += "shading.highlight=" + highlight_name(program.shading.highlight_model) + "\n";
  out += "shading.hatch=" + d2s(program.shading.hatch_density) + "\n";
  out += "shading.halftone=" + d2s(program.shading.halftone_scale) + "\n";
  out += "shading.terminator=" + d2s(program.shading.terminator_anchor) + "\n";
  out += "motion.cadence=" + d2s(program.motion.timing_cadence) + "\n";
  out += "motion.spacing=" + d2s(program.motion.spacing_behavior) + "\n";
  out += "motion.smear=" + d2s(program.motion.smear_threshold) + "\n";
  out += "motion.stepped=" + d2s(program.motion.stepped_cadence) + "\n";
  out += "motion.abstraction=" + d2s(program.motion.motion_abstraction) + "\n";
  out += "palette.policy=" + palette_policy_name(program.palette.palette_policy) + "\n";
  out += "palette.value_grouping=" + d2s(program.palette.value_grouping) + "\n";
  out += "palette.saturation=" + d2s(program.palette.saturation_behavior) + "\n";
  out += "palette.hue_shift=" + d2s(program.palette.hue_shift_curve) + "\n";
  out += "palette.max_colors=" + std::to_string(program.palette.max_colors) + "\n";
  out += "fx.aura=" + d2s(program.fx.aura_interpretation) + "\n";
  out += "fx.emission=" + d2s(program.fx.emission_interpretation) + "\n";
  out += "fx.halftone=" + d2s(program.fx.halftone_dots) + "\n";
  out += "fx.registration=" + d2s(program.fx.registration_shift) + "\n";
  out += "fx.impact_frames=" + d2s(program.fx.impact_frames) + "\n";
  out += "fx.speed_lines=" + d2s(program.fx.speed_lines) + "\n";
  out += "compositing.mode=" + compositing_name(program.compositing.layer_compositing) + "\n";
  out += "compositing.alpha=" + d2s(program.compositing.alpha_policy) + "\n";
  out += "compositing.aa=" + aa_name(program.compositing.aa_policy) + "\n";
  out += "compositing.pixel_quantization=" + d2s(program.compositing.pixel_quantization) + "\n";
  for (const auto& patch : program.patches)
    out += "patch=" + canonicalize_style_patch(patch) + "\n";
  return out;
}

std::string style_program_identity(const StyleProgram& program) {
  return gspl::sprites::sha256("style-program|" + canonicalize_style_program(program));
}

} // namespace gspl::sprites::visual
