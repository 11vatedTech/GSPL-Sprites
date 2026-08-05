#include "gspl_sprites/morphology.hpp"
#include "gspl_sprites/animation_sampling.hpp"  // canonical_double

#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\n"; break;
      case '\r': out += "\r"; break;
      case '\t': out += "\t"; break;
      default:
        if (c < 0x20) throw std::runtime_error("control character is not canonicalizable");
        out += static_cast<char>(c);
    }
  }
  return out;
}

} // namespace

std::string canonicalize_morphology_part(const MorphologyPart& part) {
  std::ostringstream out;
  out << "{\"boneId\":\"" << escape_json(part.bone_id) << "\",\"color\":\"" << escape_json(part.color) << "\",\"electricalMarking\":" << (part.electrical_marking ? "true" : "false")
      << ",\"emissive\":" << (part.emissive ? "true" : "false") << ",\"parent\":\"" << escape_json(part.parent) << "\",\"primitive\":\"" << escape_json(part.primitive)
      << "\",\"rotationDegrees\":" << part.rotation_degrees << ",\"semanticRole\":\"" << escape_json(part.semantic_role)
      << "\",\"sizeX\":" << part.size_x << ",\"sizeY\":" << part.size_y << ",\"sizeZ\":" << part.size_z
      << ",\"x\":" << part.x << ",\"y\":" << part.y << ",\"z\":" << part.z
      << ",\"zOrder\":" << part.z_order << "}";
  return out.str();
}

std::string canonicalize_morphology_map(const MorphologyMap& map) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto& [name, part] : map) {
    if (!first) out << ",";
    first = false;
    out << "\"" << escape_json(name) << "\":" << canonicalize_morphology_part(part);
  }
  out << "}";
  return out.str();
}

std::string canonicalize_form_morphology_overrides(const FormMorphologyOverrides& overrides) {
  std::ostringstream out;
  out << "{";
  bool first_form = true;
  for (const auto& [form_id, parts] : overrides) {
    if (!first_form) out << ",";
    first_form = false;
    out << "\"" << escape_json(form_id) << "\":" << canonicalize_morphology_map(parts);
  }
  out << "}";
  return out.str();
}

std::string canonicalize_effective_morphology_preimage(const MorphologyMap& morph) {
  std::string preimage;
  for (auto const& [part_id, mp] : morph) {
    preimage += part_id + "\n";
    preimage += mp.bone_id + "\n";
    preimage += mp.primitive + "\n";
    preimage += mp.semantic_role + "\n";
    preimage += mp.color + "\n";
    preimage += mp.parent + "\n";
    preimage += canonical_double(mp.x) + "," + canonical_double(mp.y) + "," + canonical_double(mp.z) + "\n";
    preimage += canonical_double(mp.size_x) + "," + canonical_double(mp.size_y) + "," + canonical_double(mp.size_z) + "\n";
    preimage += canonical_double(mp.rotation_degrees) + "\n";
    preimage += std::to_string(mp.z_order) + "\n";
    preimage += std::string(mp.emissive ? "1" : "0") + "\n";
    preimage += std::string(mp.electrical_marking ? "1" : "0") + "\n";
  }
  return preimage;
}

std::string canonicalize_transformations_preimage(std::span<const MorphologyMap> transformation) {
  std::string preimage;
  preimage += std::to_string(transformation.size()) + "\n";
  for (std::size_t i = 0; i < transformation.size(); ++i) {
    preimage += std::to_string(i) + "\n";
    preimage += canonicalize_effective_morphology_preimage(transformation[i]);
  }
  return preimage;
}

} // namespace gspl::sprites
