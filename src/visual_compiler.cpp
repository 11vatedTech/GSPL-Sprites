#include "gspl_sprites/morphology.hpp"
#include "gspl_sprites/visual_compiler.hpp"

#include <algorithm>
#include <map>

namespace gspl::sprites::visual {
namespace {

[[nodiscard]] MorphologyMap resolve_form_morphology_ir(const SpriteIr& ir, std::string_view form_id,
                                                       ValidationResult& diagnostics) {
  MorphologyMap out = ir.morphology;
  if (form_id.empty()) return out;
  const auto it = ir.form_morphology_overrides.find(std::string(form_id));
  if (it == ir.form_morphology_overrides.end()) return out;
  for (const auto& [name, override_part] : it->second) {
    if (!out.count(name)) {
      diagnostics.diagnostics.push_back({"VISUAL_OVERRIDE_PART_NOT_FOUND",
          "form '" + std::string(form_id) + "' overrides unknown part '" + name + "'"});
      continue;
    }
    out[name] = override_part;
  }
  return out;
}

[[nodiscard]] bool is_base_form(const SpriteIr& ir, std::string_view form_id) {
  if (form_id.empty()) return true;
  for (const auto& f : ir.form_definitions) {
    if (f.name == form_id) return false;  // named forms are derived (alternate palette)
  }
  return false;
}

} // namespace

VisualIrResult compile_visual_ir(const SpriteIr& ir, const VisualCompileOptions& options) {
  VisualIrResult result;
  ValidationResult& diag = result.diagnostics;

  if (ir.entity_id.empty()) {
    diag.diagnostics.push_back({"VISUAL_ENTITY_EMPTY", "SpriteIr entity_id is empty"});
    return result;
  }

  std::string form_id = options.form_id;
  if (form_id.empty() && !ir.form_definitions.empty()) {
    form_id = ir.form_definitions.front().name;
  }

  // 1. Resolve v1 morphology for the form (sealed semantics).
  MorphologyMap v1 = resolve_form_morphology_ir(ir, form_id, diag);
  if (v1.empty()) {
    diag.diagnostics.push_back({"VISUAL_MORPHOLOGY_EMPTY",
        "no morphology for form '" + form_id + "'"});
    return result;
  }

  // 2. Lower to Visual Morphology v2 (deterministic compatibility path).
  VisualMorphologyV2 morph = lower_visual_morphology_v1(v1, form_id);

  // 3. Form palette: base form uses entity colors; derived forms use the
  //    alternate (storm) palette when defined. Data-driven, no name fixed.
  const bool base = is_base_form(ir, options.form_id.empty() ? std::string_view{} : std::string_view(form_id));
  PaletteDefinition palette;
  if (base) {
    palette = make_default_palette(ir.primary_color, ir.accent_color, ir.emissive_color, ir.aura_color);
  } else {
    const std::string p = !ir.storm_primary_color.empty() ? ir.storm_primary_color : ir.accent_color;
    const std::string a = !ir.storm_accent_color.empty() ? ir.storm_accent_color : ir.primary_color;
    palette = make_transformed_palette(p, a, ir.emissive_color, ir.aura_color);
  }
  palette.form_id = form_id;

  // 4. Effective style: preset (if any) composed with explicit patches.
  StyleSemantics style = options.style_preset.empty()
      ? StyleSemantics{}
      : make_style_preset(options.style_preset);
  style = compose_style(style, options.style_patches);

  // 5. Performance: bake motions and impact into part transforms.
  PerformanceState perf;
  if (options.performance) perf = *options.performance;
  for (auto& [id, part] : morph.parts) {
    if (!options.performance) continue;
    const PerformanceState& p = *options.performance;
    for (const PartMotion& m : p.motions) {
      if (m.part_id != id) continue;
      part.x += m.dx;
      part.y += m.dy;
      part.rotation_degrees += m.rotation_degrees;
      part.size_x *= m.scale_x;
      part.size_y *= m.scale_y;
      if (m.opacity > 0.0) part.opacity = m.opacity;
    }
    const double squash = 1.0 - p.impact * 0.15;
    const double stretch = 1.0 + p.impact * 0.15;
    part.size_x *= squash;
    part.size_y *= stretch;
  }

  // 6. Materials are collected below per part (deduplicated by id).

  VisualIr ir_out;
  ir_out.schema = "gspl.visual-ir/0.1";
  ir_out.form_id = form_id;
  ir_out.projection.kind = options.projection_kind;
  ir_out.projection.width = options.canvas_width;
  ir_out.projection.height = options.canvas_height;
  ir_out.projection.mirror_x = (perf.facing == "left");
  ir_out.lighting = LightingSpec{};
  ir_out.morphology = std::move(morph);
  ir_out.style = style;
  ir_out.palette = palette;
  ir_out.performance = perf;
  ir_out.layer_order = ir_out.morphology.layer_order;

  // Materials for each part (deduplicated by id).
  std::map<std::string, std::size_t, std::less<>> mat_index;
  for (auto& [id, part] : ir_out.morphology.parts) {
    std::string mat_id = part.material_id;
    if (mat_id.empty()) {
      mat_id = ir.entity_id + "." + form_id + ".mat." + part.material_class;
      part.material_id = mat_id;
    }
    if (mat_index.find(mat_id) == mat_index.end()) {
      MaterialSemantics m = make_material(part.material_class, mat_id);
      mat_index.emplace(mat_id, ir_out.materials.size());
      ir_out.materials.push_back(std::move(m));
    }
  }

  // 7. Channel requests (default set when not supplied).
  if (options.channel_requests.empty()) {
    ir_out.channel_requests = {
        {ChannelMapKind::emissive, ir.entity_id + ".emissive"},
        {ChannelMapKind::depth, ir.entity_id + ".depth"},
        {ChannelMapKind::material_id, ir.entity_id + ".material_region"},
        {ChannelMapKind::effects, ir.entity_id + ".mask"},
    };
  } else {
    ir_out.channel_requests.assign(options.channel_requests.begin(), options.channel_requests.end());
  }

  // 8. Visual manifestation identity (binds entity + form + sealed v1 morphology).
  {
    std::string seed_part = ir.seed_identity.empty() ? ir.entity_id : ir.seed_identity;
    const std::string morph_canon = gspl::sprites::canonicalize_morphology_map(v1);
    ir_out.entity_identity = gspl::sprites::sha256(
        seed_part + "|" + ir.entity_id + "|" + form_id + "|" + morph_canon);
  }

  result.value = std::move(ir_out);
  const ValidationResult check = validate_visual_ir(*result.value);
  for (auto& d : check.diagnostics) diag.diagnostics.push_back(std::move(d));
  return result;
}

} // namespace gspl::sprites::visual
