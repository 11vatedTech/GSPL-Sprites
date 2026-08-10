#include "gspl_sprites/material.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {

MaterialSemantics make_material(std::string_view class_name, std::string id) {
  MaterialSemantics m;
  m.id = std::move(id);
  m.class_name = std::string(class_name);
  if (class_name == "fur") {
    m.roughness = 0.85; m.reflectivity = 0.05; m.subsurface_intent = 0.25;
    m.surface_texture_scale = 3.0; m.edge_response = 0.7;
  } else if (class_name == "skin") {
    m.roughness = 0.6; m.subsurface_intent = 0.35; m.edge_response = 0.6;
  } else if (class_name == "cloth") {
    m.roughness = 0.9; m.surface_texture_scale = 2.0; m.anisotropy = 0.2;
  } else if (class_name == "metal") {
    m.roughness = 0.25; m.reflectivity = 0.6; m.metallicity = 0.9;
    m.edge_response = 0.9; m.rim_response = 0.3;
  } else if (class_name == "wood") {
    m.roughness = 0.8; m.anisotropy = 0.5; m.surface_texture_scale = 2.2;
  } else if (class_name == "stone") {
    m.roughness = 0.95; m.surface_texture_scale = 2.5;
  } else if (class_name == "glass") {
    m.roughness = 0.05; m.reflectivity = 0.7; m.translucency = 0.8;
    m.edge_response = 1.0; m.rim_response = 0.5;
  } else if (class_name == "energy") {
    m.roughness = 0.1; m.emission = 0.9; m.translucency = 0.4; m.rim_response = 0.3;
  } else if (class_name == "electricity") {
    m.roughness = 0.1; m.emission = 0.85; m.rim_response = 0.4; m.edge_response = 1.0;
  } else if (class_name == "smoke") {
    m.roughness = 1.0; m.translucency = 0.95; m.emission = 0.1;
  } else if (class_name == "liquid") {
    m.roughness = 0.15; m.reflectivity = 0.5; m.translucency = 0.5;
  } else if (class_name == "plastic") {
    m.roughness = 0.4; m.reflectivity = 0.3; m.edge_response = 0.7;
  } else if (class_name == "ceramic") {
    m.roughness = 0.3; m.reflectivity = 0.4; m.edge_response = 0.8;
  }
  return m;
}

MaterialSemantics resolve_material(const MaterialSemantics& base, const StyleSemantics& style) {
  MaterialSemantics out = base;
  // Style modulates surface response deterministically; material class remains data.
  out.roughness = std::clamp(out.roughness + style.texture_density * 0.25, 0.0, 1.0);
  out.surface_texture_scale = std::max(0.5, out.surface_texture_scale * style.detail_density);
  out.emission = std::clamp(out.emission + (base.emission > 0.0 ? style.value_grouping * 0.1 : 0.0), 0.0, 1.0);
  return out;
}

ValidationResult validate_material(const MaterialSemantics& material) {
  ValidationResult result;
  auto range = [&](const char* code, double v, double lo, double hi) {
    if (!(v >= lo && v <= hi)) {
      result.diagnostics.push_back({code, "value " + std::to_string(v) + " outside [" + std::to_string(lo) + "," + std::to_string(hi) + "]"});
    }
  };
  if (material.schema != "gspl.material/0.1") {
    result.diagnostics.push_back({"MATERIAL_SCHEMA", "unexpected material schema"});
  }
  if (material.class_name.empty()) {
    result.diagnostics.push_back({"MATERIAL_CLASS_EMPTY", "material class must not be empty"});
  }
  range("MATERIAL_ROUGHNESS", material.roughness, 0.0, 1.0);
  range("MATERIAL_REFLECTIVITY", material.reflectivity, 0.0, 1.0);
  range("MATERIAL_METALLICITY", material.metallicity, 0.0, 1.0);
  range("MATERIAL_TRANSLUCENCY", material.translucency, 0.0, 1.0);
  range("MATERIAL_EMISSION", material.emission, 0.0, 1.0);
  range("MATERIAL_ANISOTROPY", material.anisotropy, 0.0, 1.0);
  range("MATERIAL_EDGE", material.edge_response, 0.0, 1.0);
  range("MATERIAL_RIM", material.rim_response, 0.0, 1.0);
  range("MATERIAL_SUBSURFACE", material.subsurface_intent, 0.0, 1.0);
  if (!(material.surface_texture_scale >= 0.0)) {
    result.diagnostics.push_back({"MATERIAL_TEXTURE_SCALE", "surface_texture_scale must be >= 0"});
  }
  return result;
}

std::string canonicalize_material(const MaterialSemantics& m) {
  std::ostringstream out;
  out << "{\"schema\":\"" << m.schema << "\",\"id\":\"" << m.id
      << "\",\"class\":\"" << m.class_name << "\""
      << ",\"role\":\"" << color_role_name(m.base_color_role) << "\""
      << ",\"rough\":" << m.roughness
      << ",\"refl\":" << m.reflectivity
      << ",\"metal\":" << m.metallicity
      << ",\"transl\":" << m.translucency
      << ",\"emiss\":" << m.emission
      << ",\"tex\":" << m.surface_texture_scale
      << ",\"aniso\":" << m.anisotropy
      << ",\"edge\":" << m.edge_response
      << ",\"rim\":" << m.rim_response
      << ",\"sss\":" << m.subsurface_intent << "}";
  return out.str();
}

} // namespace gspl::sprites::visual
