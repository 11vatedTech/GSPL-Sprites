#include "gspl_sprites/core.hpp"
#include "gspl_sprites/style.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

void append_opt_double(std::ostringstream& out, const char* key, const std::optional<double>& v) {
  if (v) out << ",\"" << key << "\":" << *v;
}

template <typename T>
void append_opt_enum(std::ostringstream& out, const char* key, const std::optional<T>& v) {
  if (v) out << ",\"" << key << "\":" << static_cast<int>(*v);
}

} // namespace

StyleSemantics make_style_preset(std::string_view name) {
  StyleSemantics s;
  if (name == "clean-flat") {
    s.shape_abstraction = 0.5;
    s.line_weight = 0.0;
    s.outline_selection = OutlineSelection::none;
    s.shadow_model = ShadowModel::flat;
    s.highlight_model = HighlightModel::none;
    s.curve_smoothness = 0.7;
    s.aa_policy = AntiAliasPolicy::analytic;
  } else if (name == "inked") {
    s.shape_abstraction = 0.4;
    s.contour = ContourBehavior::angular;
    s.line_weight = 2.2;
    s.line_taper = 0.4;
    s.outline_selection = OutlineSelection::silhouette;
    s.corner_sharpness = 0.85;
    s.shadow_model = ShadowModel::flat;
    s.value_grouping = 0.2;
    s.saturation_behavior = 1.1;
    s.aa_policy = AntiAliasPolicy::analytic;
  } else if (name == "soft-shaded") {
    s.line_weight = 0.8;
    s.outline_selection = OutlineSelection::silhouette;
    s.shadow_model = ShadowModel::gradient;
    s.highlight_model = HighlightModel::strong;
    s.edge_softness = 0.6;
    s.curve_smoothness = 0.85;
    s.aa_policy = AntiAliasPolicy::analytic;
  } else if (name == "pixel-constrained") {
    s.shape_abstraction = 0.7;
    s.line_weight = 1.0;
    s.outline_selection = OutlineSelection::silhouette;
    s.palette_policy = PalettePolicy::quantize;
    s.shadow_model = ShadowModel::flat;
    s.value_grouping = 0.35;
    s.aa_policy = AntiAliasPolicy::none;
    s.pixel_quantization = 8.0;
    s.corner_sharpness = 1.0;
  }
  return s;
}

StyleSemantics compose_style(const StyleSemantics& base, std::span<const StylePatch> patches) {
  StyleSemantics out = base;
  for (const StylePatch& p : patches) {
    if (p.shape_abstraction) out.shape_abstraction = *p.shape_abstraction;
    if (p.proportion_exaggeration) out.proportion_exaggeration = *p.proportion_exaggeration;
    if (p.contour) out.contour = *p.contour;
    if (p.line_weight) out.line_weight = *p.line_weight;
    if (p.line_taper) out.line_taper = *p.line_taper;
    if (p.outline_selection) out.outline_selection = *p.outline_selection;
    if (p.corner_sharpness) out.corner_sharpness = *p.corner_sharpness;
    if (p.curve_smoothness) out.curve_smoothness = *p.curve_smoothness;
    if (p.palette_policy) out.palette_policy = *p.palette_policy;
    if (p.value_grouping) out.value_grouping = *p.value_grouping;
    if (p.saturation_behavior) out.saturation_behavior = *p.saturation_behavior;
    if (p.shadow_model) out.shadow_model = *p.shadow_model;
    if (p.highlight_model) out.highlight_model = *p.highlight_model;
    if (p.texture_density) out.texture_density = *p.texture_density;
    if (p.detail_density) out.detail_density = *p.detail_density;
    if (p.edge_softness) out.edge_softness = *p.edge_softness;
    if (p.aa_policy) out.aa_policy = *p.aa_policy;
    if (p.pixel_quantization) out.pixel_quantization = *p.pixel_quantization;
    if (p.layer_compositing) out.layer_compositing = *p.layer_compositing;
  }
  return out;
}

ValidationResult validate_style(const StyleSemantics& style) {
  ValidationResult result;
  auto range = [&](const char* code, double v, double lo, double hi) {
    if (!(v >= lo && v <= hi)) {
      result.diagnostics.push_back({code, "value " + std::to_string(v) + " outside [" + std::to_string(lo) + "," + std::to_string(hi) + "]"});
    }
  };
  if (style.schema != "gspl.style/0.1") {
    result.diagnostics.push_back({"STYLE_SCHEMA", "unexpected style schema"});
  }
  range("STYLE_ABSTRACTION", style.shape_abstraction, 0.0, 1.0);
  range("STYLE_PROPORTION", style.proportion_exaggeration, -1.0, 1.0);
  range("STYLE_LINE_WEIGHT", style.line_weight, 0.0, 64.0);
  range("STYLE_LINE_TAPER", style.line_taper, 0.0, 1.0);
  range("STYLE_CORNER", style.corner_sharpness, 0.0, 1.0);
  range("STYLE_CURVE", style.curve_smoothness, 0.0, 1.0);
  range("STYLE_VALUE_GROUP", style.value_grouping, 0.0, 1.0);
  range("STYLE_SATURATION", style.saturation_behavior, 0.0, 2.0);
  range("STYLE_TEXTURE", style.texture_density, 0.0, 1.0);
  range("STYLE_DETAIL", style.detail_density, 0.0, 2.0);
  range("STYLE_EDGE_SOFT", style.edge_softness, 0.0, 4.0);
  range("STYLE_QUANTIZATION", style.pixel_quantization, 0.0, 1024.0);
  // Renderer-affecting fields must be validated AND canonicalized so they
  // participate in style_identity and therefore visual_ir_identity.
  if (style.band_count < 1)
    result.diagnostics.push_back({"STYLE_BAND_COUNT", "band_count must be >= 1 (got " +
                                  std::to_string(style.band_count) + ")"});
  if (style.band_count > 256)
    result.diagnostics.push_back({"STYLE_BAND_COUNT_LIMIT", "band_count exceeds resource bound 256"});
  if (style.max_colors != 0 && style.max_colors < 2)
    result.diagnostics.push_back({"STYLE_MAX_COLORS", "max_colors must be 0 (unlimited) or >= 2 (got " +
                                  std::to_string(style.max_colors) + ")"});
  if (style.max_colors > 4096)
    result.diagnostics.push_back({"STYLE_MAX_COLORS_LIMIT", "max_colors exceeds resource bound 4096"});
  return result;
}

std::string canonicalize_style(const StyleSemantics& s) {
  std::ostringstream out;
  out << "{\"schema\":\"" << s.schema << "\""
      << ",\"abstraction\":" << s.shape_abstraction
      << ",\"proportion\":" << s.proportion_exaggeration
      << ",\"contour\":" << static_cast<int>(s.contour)
      << ",\"line_weight\":" << s.line_weight
      << ",\"line_taper\":" << s.line_taper
      << ",\"outline\":" << static_cast<int>(s.outline_selection)
      << ",\"corner\":" << s.corner_sharpness
      << ",\"curve\":" << s.curve_smoothness
      << ",\"palette\":" << static_cast<int>(s.palette_policy)
      << ",\"value_group\":" << s.value_grouping
      << ",\"saturation\":" << s.saturation_behavior
      << ",\"shadow\":" << static_cast<int>(s.shadow_model)
      << ",\"highlight\":" << static_cast<int>(s.highlight_model)
      << ",\"texture\":" << s.texture_density
      << ",\"detail\":" << s.detail_density
      << ",\"edge_soft\":" << s.edge_softness
      << ",\"aa\":" << static_cast<int>(s.aa_policy)
      << ",\"quant\":" << s.pixel_quantization
      << ",\"composite\":" << static_cast<int>(s.layer_compositing)
      << ",\"bands\":" << s.band_count
      << ",\"max_colors\":" << s.max_colors
      << "}";
  return out.str();
}

std::string canonicalize_style_patch(const StylePatch& p) {
  std::ostringstream out;
  out << "{\"scope\":\"" << p.scope << "\"";
  append_opt_double(out, "abstraction", p.shape_abstraction);
  append_opt_double(out, "proportion", p.proportion_exaggeration);
  append_opt_enum(out, "contour", p.contour);
  append_opt_double(out, "line_weight", p.line_weight);
  append_opt_double(out, "line_taper", p.line_taper);
  append_opt_enum(out, "outline", p.outline_selection);
  append_opt_double(out, "corner", p.corner_sharpness);
  append_opt_double(out, "curve", p.curve_smoothness);
  append_opt_enum(out, "palette", p.palette_policy);
  append_opt_double(out, "value_group", p.value_grouping);
  append_opt_double(out, "saturation", p.saturation_behavior);
  append_opt_enum(out, "shadow", p.shadow_model);
  append_opt_enum(out, "highlight", p.highlight_model);
  append_opt_double(out, "texture", p.texture_density);
  append_opt_double(out, "detail", p.detail_density);
  append_opt_double(out, "edge_soft", p.edge_softness);
  append_opt_enum(out, "aa", p.aa_policy);
  append_opt_double(out, "quant", p.pixel_quantization);
  append_opt_enum(out, "composite", p.layer_compositing);
  out << "}";
  return out.str();
}

std::string style_identity(const StyleSemantics& style) {
  return gspl::sprites::sha256(canonicalize_style(style));
}

} // namespace gspl::sprites::visual
