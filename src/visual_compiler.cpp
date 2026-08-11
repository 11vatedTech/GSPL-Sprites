#include "gspl_sprites/morphology.hpp"
#include "gspl_sprites/performance_intent.hpp"
#include "gspl_sprites/style_program.hpp"
#include "gspl_sprites/visual_canon.hpp"
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

  // 1. Resolve v1 morphology for the form (sealed semantics) when no
  //    VisualCanon is supplied; a canon drives construction instead.
  MorphologyMap v1;
  bool canon_driven = false;
  std::string canon_identity;
  if (options.canon) {
    // Canon-driven construction: validated above in the caller or here.
    canon_driven = true;
    const ValidationResult canon_check = validate_visual_canon(*options.canon);
    for (const auto& d : canon_check.diagnostics) diag.diagnostics.push_back(d);
    if (!canon_check.ok()) return result;
  } else {
    v1 = resolve_form_morphology_ir(ir, form_id, diag);
    if (v1.empty()) {
      diag.diagnostics.push_back({"VISUAL_MORPHOLOGY_EMPTY",
          "no morphology for form '" + form_id + "'"});
      return result;
    }
  }

  // 2. Morphology: canon-driven (Visual Morphology v2 construction) or the
  //    deterministic v1 -> v2 compatibility lowering. Canon coordinates are
  //    authored in raster projection units (position x3 px, size x1.5 px);
  //    body_scale is 1.0 for the sealed projection contract.
  VisualMorphologyV2 morph = canon_driven
      ? canon_to_morphology(*options.canon, form_id, 1.0)
      : lower_visual_morphology_v1(v1, form_id);

  // 2b. Canon-driven identity invariants validate the CANONICAL construction
  //     before any pose/performance application: identity is a property of
  //     what the entity is, enforced by check_identity_invariants here;
  //     how far a pose may deviate is bounded separately by the deformation
  //     envelopes (step 5). Hard invariant failures fail closed.
  if (canon_driven) {
    const ValidationResult inv = check_identity_invariants(*options.canon, morph);
    for (const auto& d : inv.diagnostics) diag.diagnostics.push_back(d);
    if (!inv.ok()) return result;
  }

  // 3. Form palette: canon palettes when canon-driven; otherwise the sealed
  //    base/derived palette decision (data-driven, no name fixed).
  PaletteDefinition palette;
  if (canon_driven) {
    palette.schema = "gspl.palette/0.1";
    for (const auto& [role, hex] : options.canon->base_palette) {
      if (const auto c = parse_hex_color(hex)) palette.colors[role] = *c;
    }
    const auto fit = options.canon->form_palettes.find(form_id);
    if (fit != options.canon->form_palettes.end()) {
      for (const auto& [role, hex] : fit->second) {
        if (const auto c = parse_hex_color(hex)) palette.colors[role] = *c;
      }
    }
    palette.form_id = form_id;
  } else {
    const bool base = is_base_form(ir, options.form_id.empty() ? std::string_view{} : std::string_view(form_id));
    if (base) {
      palette = make_default_palette(ir.primary_color, ir.accent_color, ir.emissive_color, ir.aura_color);
    } else {
      const std::string p = !ir.storm_primary_color.empty() ? ir.storm_primary_color : ir.accent_color;
      const std::string a = !ir.storm_accent_color.empty() ? ir.storm_accent_color : ir.primary_color;
      palette = make_transformed_palette(p, a, ir.emissive_color, ir.aura_color);
    }
    palette.form_id = form_id;
  }

  // 4. Effective style: factorized StyleProgram wins over preset+patches;
  //    both paths lower deterministically to StyleSemantics.
  StyleSemantics style;
  if (options.style_program) {
    style = effective_style(*options.style_program);
  } else {
    style = options.style_preset.empty()
        ? StyleSemantics{}
        : make_style_preset(options.style_preset);
    style = compose_style(style, options.style_patches);
  }

  // 5. Performance: semantic intent (with optional key pose) lowers to the
  //    PerformanceState consumed below; a direct state still wins if given.
  PerformanceState perf;
  if (options.performance) {
    perf = *options.performance;
  } else if (options.intent) {
    perf = pose_to_performance_state(*options.intent, options.key_pose);
    // Canon-driven deformation enforcement: clamp motions per envelope;
    // hard-boundary violations fail closed.
    if (canon_driven) {
      const double expression_factor =
          perf.expression_intent.empty() ? 0.0 : 0.5;
      const ValidationResult env = enforce_deformation_envelopes(
          *options.canon, perf,
          perf.action_phase, expression_factor, 1.0, canon_driven ? 1.0 : 0.0);
      // Deviation beyond 'allowed' is clamped in-place (permitted, per the
      // envelope contract) and reported only as an informational note; only
      // hard-boundary / rigid violations are identity-critical and fail
      // closed. Informational notes never poison VisualIrResult::ok().
      bool identity_violation = false;
      for (const auto& d : env.diagnostics) {
        if (d.code == "DEFORMATION_HARD_VIOLATION" || d.code == "DEFORMATION_RIGID_VIOLATION") {
          identity_violation = true;
          diag.diagnostics.push_back(d);
        }
      }
      if (identity_violation) return result;
    }
  }
  // Apply the effective performance state (direct state or intent-derived)
  // to the constructed morphology. Default state has no motions and zero
  // impact, so this is a no-op when no performance is supplied.
  for (auto& [id, part] : morph.parts) {
    for (const PartMotion& m : perf.motions) {
      if (m.part_id != id) continue;
      part.x += m.dx;
      part.y += m.dy;
      part.rotation_degrees += m.rotation_degrees;
      part.size_x *= m.scale_x;
      part.size_y *= m.scale_y;
      if (m.opacity > 0.0) part.opacity = m.opacity;
    }
    const double squash = 1.0 - perf.impact * 0.15;
    const double stretch = 1.0 + perf.impact * 0.15;
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

  // 8. Visual manifestation identity (binds entity + form + sealed v1
  //    morphology or the canon-driven construction).
  {
    std::string seed_part = ir.seed_identity.empty() ? ir.entity_id : ir.seed_identity;
    if (canon_driven) {
      canon_identity = visual_canon_identity(*options.canon);
      ir_out.entity_identity = gspl::sprites::sha256(
          seed_part + "|" + ir.entity_id + "|" + form_id + "|canon|" + canon_identity);
    } else {
      const std::string morph_canon = gspl::sprites::canonicalize_morphology_map(v1);
      ir_out.entity_identity = gspl::sprites::sha256(
          seed_part + "|" + ir.entity_id + "|" + form_id + "|" + morph_canon);
    }
  }

  result.value = std::move(ir_out);
  const ValidationResult check = validate_visual_ir(*result.value);
  for (auto& d : check.diagnostics) diag.diagnostics.push_back(std::move(d));
  return result;
}

} // namespace gspl::sprites::visual
