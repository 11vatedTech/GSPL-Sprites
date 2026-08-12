#include "gspl_sprites/visual_ir.hpp"
#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <sstream>

namespace gspl::sprites::visual {

ValidationResult validate_visual_ir(const VisualIr& ir, const VisualLimits& limits) {
  ValidationResult result;
  auto add = [&](std::string code, const std::string& msg) {
    result.diagnostics.push_back({std::move(code), msg});
  };

  if (ir.schema != "gspl.visual-ir/0.1") {
    add("VISUAL_IR_SCHEMA", "unexpected visual IR schema '" + ir.schema + "'");
  }
  if (ir.entity_identity.empty()) {
    add("VISUAL_IR_IDENTITY_EMPTY", "visual IR requires an entity_identity");
  }
  if (ir.projection.kind != "2d" && ir.projection.kind != "2.5d" && ir.projection.kind != "3d") {
    add("VISUAL_IR_PROJECTION", "unsupported projection kind '" + ir.projection.kind + "'");
  }
  if (ir.projection.width == 0 || ir.projection.height == 0) {
    add("VISUAL_IR_CANVAS_ZERO", "canvas dimensions must be non-zero");
  }
  if (ir.projection.width > limits.max_canvas_width || ir.projection.height > limits.max_canvas_height) {
    add("VISUAL_IR_CANVAS_LIMIT",
        "canvas " + std::to_string(ir.projection.width) + "x" + std::to_string(ir.projection.height) +
        " exceeds limit " + std::to_string(limits.max_canvas_width) + "x" + std::to_string(limits.max_canvas_height));
  }
  if (ir.materials.size() > limits.max_materials) {
    add("VISUAL_IR_MATERIAL_LIMIT", "material count exceeds " + std::to_string(limits.max_materials));
  }
  if (ir.channel_requests.size() > limits.max_channel_requests) {
    add("VISUAL_IR_CHANNEL_LIMIT", "channel request count exceeds " + std::to_string(limits.max_channel_requests));
  }

  std::vector<std::string> material_ids;
  for (const auto& m : ir.materials) {
    material_ids.push_back(m.id);
    const auto mv = validate_material(m);
    for (auto& d : mv.diagnostics) result.diagnostics.push_back(std::move(d));
  }
  std::ranges::sort(material_ids);
  for (std::size_t i = 1; i < material_ids.size(); ++i) {
    if (material_ids[i] == material_ids[i - 1]) {
      add("VISUAL_IR_DUPLICATE_MATERIAL", "duplicate material id '" + material_ids[i] + "'");
    }
  }

  std::vector<std::string> part_ids;
  part_ids.reserve(ir.morphology.parts.size());
  for (const auto& [id, part] : ir.morphology.parts) part_ids.push_back(id);

  // Material references: explicit material_id must exist in the resolved set.
  for (const auto& [id, part] : ir.morphology.parts) {
    if (!part.material_id.empty() &&
        std::none_of(ir.materials.begin(), ir.materials.end(),
                     [&](const MaterialSemantics& m) { return m.id == part.material_id; })) {
      add("VISUAL_IR_MATERIAL_REF", "part '" + id + "' references unknown material '" + part.material_id + "'");
    }
  }

  const auto morph_result = validate_visual_morphology(ir.morphology, limits.max_parts);
  for (auto& d : morph_result.diagnostics) result.diagnostics.push_back(std::move(d));

  const auto style_result = validate_style(ir.style);
  for (auto& d : style_result.diagnostics) result.diagnostics.push_back(std::move(d));

  const auto palette_result = validate_palette(ir.palette);
  for (auto& d : palette_result.diagnostics) result.diagnostics.push_back(std::move(d));

  const auto perf_result = validate_performance(ir.performance, part_ids);
  for (auto& d : perf_result.diagnostics) result.diagnostics.push_back(std::move(d));

  if (ir.layer_order.empty()) {
    add("VISUAL_IR_LAYER_ORDER_EMPTY", "layer_order must not be empty");
  }
  for (const std::string& layer : ir.layer_order) {
    if (!layer_from_name(layer)) {
      add("VISUAL_IR_UNKNOWN_LAYER", "unknown layer '" + layer + "'");
    }
  }

  for (const auto& cr : ir.channel_requests) {
    if (cr.id.empty()) {
      add("VISUAL_IR_CHANNEL_ID", "channel request has an empty id");
    }
  }

  // Validate FX state embedded in IR (if present).
  if (!ir.fx_state.entity_id.empty()) {
    const auto fx_result = validate_fx_state(ir.fx_state, limits.fx_limits);
    for (auto& d : fx_result.diagnostics) result.diagnostics.push_back(std::move(d));
  }
  return result;
}

std::string canonicalize_visual_ir(const VisualIr& ir) {
  std::ostringstream out;
  out << "{\"schema\":\"" << ir.schema
      << "\",\"entity\":\"" << ir.entity_identity
      << "\",\"form\":\"" << ir.form_id << "\"";
  out << ",\"proj\":{\"kind\":\"" << ir.projection.kind
      << "\",\"w\":" << ir.projection.width
      << ",\"h\":" << ir.projection.height
      << ",\"mirror\":" << (ir.projection.mirror_x ? "true" : "false") << "}";
  out << ",\"light\":{\"deg\":" << ir.lighting.light_degrees
      << ",\"amb\":" << ir.lighting.ambient
      << ",\"key\":" << ir.lighting.key
      << ",\"fill\":" << ir.lighting.fill
      << ",\"rim\":" << ir.lighting.rim
      << ",\"shadow\":" << (ir.lighting.drop_shadow ? "true" : "false")
      << ",\"sox\":" << ir.lighting.shadow_offset_x
      << ",\"soy\":" << ir.lighting.shadow_offset_y
      << ",\"sa\":" << ir.lighting.shadow_alpha << "}";
  out << ",\"style\":" << canonicalize_style(ir.style);
  out << ",\"palette\":" << canonicalize_palette(ir.palette);
  out << ",\"performance\":" << canonicalize_performance(ir.performance);
  out << ",\"morphology\":" << canonicalize_visual_morphology(ir.morphology);
  out << ",\"materials\":[";
  std::vector<const MaterialSemantics*> ordered;
  for (const auto& m : ir.materials) ordered.push_back(&m);
  std::ranges::sort(ordered, [](const MaterialSemantics* a, const MaterialSemantics* b) {
    return a->id < b->id;
  });
  bool first_mat = true;
  for (const auto* m : ordered) {
    if (!first_mat) out << ",";
    first_mat = false;
    out << canonicalize_material(*m);
  }
  out << "]";
  out << ",\"layer_order\":[";
  for (std::size_t i = 0; i < ir.layer_order.size(); ++i) {
    if (i) out << ",";
    out << "\"" << ir.layer_order[i] << "\"";
  }
  out << "]";
  out << ",\"channels\":[";
  for (std::size_t i = 0; i < ir.channel_requests.size(); ++i) {
    if (i) out << ",";
    out << "{\"kind\":" << static_cast<int>(ir.channel_requests[i].kind)
        << ",\"id\":\"" << ir.channel_requests[i].id << "\"}";
  }
  out << "]";  // close channels array
  // Temporal part labels (per-part identity for frame correspondence)
  out << ",\"temporal_labels\":[";
  bool first_tl = true;
  for (const auto& [part_id, label] : ir.temporal_part_labels) {
    if (!first_tl) out << ",";
    first_tl = false;
    out << "{\"p\":\"" << part_id << "\",\"l\":\"" << label << "\"}";
  }
  out << "]";
  // FX state (if populated) is canonical manifestation state
  out << ",\"fx\":" << canonicalize_fx_state(ir.fx_state);
  out << "}";  // close root JSON object
  return out.str();
}

std::string visual_ir_identity(const VisualIr& ir) {
  return gspl::sprites::sha256(canonicalize_visual_ir(ir));
}

} // namespace gspl::sprites::visual
