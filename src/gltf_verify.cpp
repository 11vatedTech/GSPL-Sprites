#include "gspl_sprites/gltf_verify.hpp"

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/target_evidence.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace gspl::sprites {
namespace {
std::uint32_t u32(std::span<const std::byte> bytes, std::size_t offset) {
  if (offset + 4 > bytes.size())
    throw std::runtime_error("GLB integer is truncated");
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::string_view object_value(std::string_view json, std::string_view key) {
  const auto marker = "\"" + std::string(key) + "\":";
  const auto position = json.find(marker);
  if (position == std::string_view::npos ||
      json.find(marker, position + marker.size()) != std::string_view::npos)
    throw std::runtime_error("GLB metadata key is absent or duplicated: " +
                             std::string(key));
  const auto start = position + marker.size();
  if (start >= json.size() || json[start] != '{')
    throw std::runtime_error("GLB metadata value is not an object");
  std::size_t depth{};
  bool in_string{};
  bool escaped{};
  for (std::size_t index = start; index < json.size(); ++index) {
    const auto character = json[index];
    if (in_string) {
      if (escaped)
        escaped = false;
      else if (character == '\\')
        escaped = true;
      else if (character == '"')
        in_string = false;
      continue;
    }
    if (character == '"') {
      in_string = true;
    } else if (character == '{') {
      ++depth;
    } else if (character == '}' && --depth == 0) {
      return json.substr(start, index - start + 1);
    }
  }
  throw std::runtime_error("GLB metadata object is truncated");
}

bool has_structure(TargetFeature feature, std::string_view json) {
  switch (feature) {
  case TargetFeature::mesh_3d:
    return json.find("\"meshes\":[") != std::string_view::npos;
  case TargetFeature::pbr_materials_3d:
    return json.find("\"materials\":[") != std::string_view::npos;
  case TargetFeature::skeleton_3d:
    return json.find("\"skins\":[") != std::string_view::npos;
  case TargetFeature::morph_targets_3d:
    return json.find("\"targets\":[") != std::string_view::npos;
  case TargetFeature::animation_3d:
    return json.find("\"animations\":[") != std::string_view::npos;
  case TargetFeature::lod_3d:
    return json.find("\"gsplLodLevel\":") != std::string_view::npos;
  default:
    return false;
  }
}
} // namespace

GltfVerification verify_projection3d_glb(std::span<const std::byte> glb,
                                         std::uint64_t maximum_bytes) {
  GltfVerification result;
  const auto add = [&](std::string code, std::string message) {
    result.validation.diagnostics.push_back(
        {std::move(code), std::move(message)});
  };
  try {
    if (maximum_bytes < 20 || glb.size() > maximum_bytes || glb.size() < 20 ||
        glb.size() % 4 != 0)
      throw std::runtime_error("GLB size is invalid");
    if (u32(glb, 0) != 0x46546c67U || u32(glb, 4) != 2 ||
        u32(glb, 8) != glb.size() || u32(glb, 16) != 0x4e4f534aU)
      throw std::runtime_error("GLB header or JSON chunk is invalid");
    const auto json_length = u32(glb, 12);
    if (json_length == 0 || json_length > glb.size() - 20 ||
        json_length % 4 != 0)
      throw std::runtime_error("GLB JSON chunk length is invalid");
    const auto json_bytes = glb.subspan(20, json_length);
    const std::string_view json(
        reinterpret_cast<const char *>(json_bytes.data()), json_bytes.size());
    auto cursor = 20ULL + json_length;
    if (cursor < glb.size()) {
      if (glb.size() - cursor < 8 || u32(glb, cursor + 4) != 0x004e4942U)
        throw std::runtime_error("GLB BIN chunk is invalid");
      const auto binary_length = u32(glb, cursor);
      cursor += 8ULL + binary_length;
      if (cursor != glb.size() || binary_length % 4 != 0)
        throw std::runtime_error("GLB BIN chunk length is invalid");
    }
    const auto source = object_value(json, "gsplSourceEvidence");
    if (!is_canonical_target_source_evidence(source))
      add("SPRITE_GLB_SOURCE_EVIDENCE_INVALID",
          "GLB source evidence is not canonical");
    const auto requirement_source =
        object_value(json, "gsplTargetRequirements");
    result.requirements = parse_target_requirements(requirement_source);
    const auto report_source = object_value(json, "gsplTargetReport");
    const auto expected_report =
        canonicalize_target_compatibility(evaluate_target_compatibility(
            builtin_target_adapter("glb-2.0"), result.requirements));
    if (report_source != expected_report)
      add("SPRITE_GLB_TARGET_REPORT_MISMATCH",
          "GLB target report does not match normalized requirements");
    for (const auto &requirement : result.requirements)
      if (requirement.required && !has_structure(requirement.feature, json))
        add("SPRITE_GLB_REQUIRED_STRUCTURE_MISSING",
            "GLB lacks required target structure: " +
                std::string(target_feature_name(requirement.feature)));
    result.glb_identity = sha256(std::string_view(
        reinterpret_cast<const char *>(glb.data()), glb.size()));
  } catch (const std::exception &error) {
    add("SPRITE_GLB_MALFORMED", error.what());
  }
  return result;
}

} // namespace gspl::sprites
