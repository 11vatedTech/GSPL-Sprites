#include "gspl_sprites/target_contract.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
constexpr std::array names{
    "canonical-seed",   "rights-and-provenance", "raster-2d",
    "skeletal-2d",      "animation-graph",       "collision-2d",
    "channel-maps",     "living-runtime",        "deterministic-replay",
    "depth-planes-25d", "multi-angle-25d",       "hybrid-geometry-25d",
    "mesh-3d",          "pbr-materials-3d",      "skeleton-3d",
    "morph-targets-3d", "animation-3d",          "lod-3d",
    "engine-project"};

bool stable_id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char character) {
           return std::isalnum(character) || character == '.' ||
                  character == '_' || character == '-';
         });
}

std::string_view support_name(TargetSupport support) {
  switch (support) {
  case TargetSupport::native:
    return "native";
  case TargetSupport::adapter_emulated:
    return "adapter-emulated";
  case TargetSupport::unsupported:
    return "unsupported";
  }
  return "unsupported";
}

std::string escape_json(std::string_view value) {
  std::string result;
  for (const char character : value) {
    if (character == '"' || character == '\\')
      result += '\\';
    result += character;
  }
  return result;
}

TargetCapability capability(TargetFeature feature, TargetSupport support,
                            std::string evidence) {
  return {feature, support, std::move(evidence)};
}
} // namespace

std::string_view target_feature_name(TargetFeature feature) noexcept {
  const auto index = static_cast<std::size_t>(feature);
  return index < names.size() ? names[index] : "unknown";
}

std::optional<TargetFeature>
parse_target_feature(std::string_view name) noexcept {
  const auto found = std::ranges::find(names, name);
  if (found == names.end())
    return {};
  return static_cast<TargetFeature>(std::distance(names.begin(), found));
}

ValidationResult
validate_target_adapter(const TargetAdapterDescriptor &adapter) {
  ValidationResult result;
  if (!stable_id(adapter.id) || !stable_id(adapter.contract_version))
    result.diagnostics.push_back({"SPRITE_TARGET_ADAPTER_ID_INVALID",
                                  "target adapter identity is invalid"});
  std::set<TargetFeature> features;
  for (const auto &item : adapter.capabilities) {
    if (!features.insert(item.feature).second)
      result.diagnostics.push_back(
          {"SPRITE_TARGET_CAPABILITY_DUPLICATE",
           "target capability is duplicated: " +
               std::string(target_feature_name(item.feature))});
    if (item.evidence.empty() || item.evidence.size() > 512)
      result.diagnostics.push_back(
          {"SPRITE_TARGET_EVIDENCE_INVALID",
           "target capability evidence is absent or too large"});
  }
  return result;
}

TargetCompatibilityReport
evaluate_target_compatibility(const TargetAdapterDescriptor &adapter,
                              std::span<const TargetRequirement> requirements) {
  TargetCompatibilityReport report{adapter.id, adapter.contract_version};
  report.validation = validate_target_adapter(adapter);
  if (!report.validation.ok())
    return report;
  std::map<TargetFeature, const TargetCapability *> capabilities;
  for (const auto &item : adapter.capabilities)
    capabilities.emplace(item.feature, &item);
  std::set<TargetFeature> seen;
  for (const auto &requirement : requirements) {
    if (target_feature_name(requirement.feature) == "unknown") {
      report.validation.diagnostics.push_back(
          {"SPRITE_TARGET_REQUIREMENT_INVALID",
           "target requirement contains an unknown feature value"});
      continue;
    }
    if (!seen.insert(requirement.feature).second) {
      report.validation.diagnostics.push_back(
          {"SPRITE_TARGET_REQUIREMENT_DUPLICATE",
           "target requirement is duplicated: " +
               std::string(target_feature_name(requirement.feature))});
      continue;
    }
    const auto found = capabilities.find(requirement.feature);
    const auto support = found == capabilities.end()
                             ? TargetSupport::unsupported
                             : found->second->support;
    const auto evidence =
        found == capabilities.end()
            ? std::string("adapter does not declare this feature")
            : found->second->evidence;
    report.resolutions.push_back(
        {requirement.feature, requirement.required, support, evidence});
    if (requirement.required && support == TargetSupport::unsupported)
      report.validation.diagnostics.push_back(
          {"SPRITE_TARGET_FEATURE_UNSUPPORTED",
           "required target feature is unsupported: " +
               std::string(target_feature_name(requirement.feature))});
  }
  std::ranges::sort(report.resolutions, {}, &TargetFeatureResolution::feature);
  return report;
}

std::string
canonicalize_target_compatibility(const TargetCompatibilityReport &report) {
  auto resolutions = report.resolutions;
  std::ranges::sort(resolutions, {}, &TargetFeatureResolution::feature);
  std::ostringstream output;
  output << "{\"adapterId\":\"" << escape_json(report.adapter_id)
         << "\",\"compatible\":" << (report.compatible() ? "true" : "false")
         << ",\"contractVersion\":\"" << escape_json(report.contract_version)
         << "\",\"features\":[";
  for (std::size_t index = 0; index < resolutions.size(); ++index) {
    if (index != 0)
      output << ',';
    const auto &item = resolutions[index];
    output << "{\"evidence\":\"" << escape_json(item.evidence)
           << "\",\"feature\":\"" << target_feature_name(item.feature)
           << "\",\"required\":" << (item.required ? "true" : "false")
           << ",\"support\":\"" << support_name(item.support) << "\"}";
  }
  output << "]}";
  return output.str();
}

std::string canonicalize_target_requirements(
    std::span<const TargetRequirement> requirements) {
  if (requirements.size() > 256)
    throw std::invalid_argument("target requirement count exceeds limit");
  auto sorted =
      std::vector<TargetRequirement>(requirements.begin(), requirements.end());
  std::ranges::sort(sorted, {}, &TargetRequirement::feature);
  if (std::ranges::adjacent_find(sorted, {}, &TargetRequirement::feature) !=
      sorted.end())
    throw std::invalid_argument("target requirements contain duplicates");
  std::ostringstream output;
  output << "{\"requirements\":[";
  for (std::size_t index = 0; index < sorted.size(); ++index) {
    if (target_feature_name(sorted[index].feature) == "unknown")
      throw std::invalid_argument("target requirement feature is invalid");
    if (index)
      output << ',';
    output << "{\"feature\":\"" << target_feature_name(sorted[index].feature)
           << "\",\"required\":" << (sorted[index].required ? "true" : "false")
           << '}';
  }
  output << "]}";
  return output.str();
}

std::vector<TargetRequirement>
parse_target_requirements(std::string_view source) {
  constexpr std::string_view prefix{"{\"requirements\":["};
  if (!source.starts_with(prefix) || !source.ends_with("]}"))
    throw std::invalid_argument("target requirements document is malformed");
  std::vector<TargetRequirement> result;
  auto payload =
      source.substr(prefix.size(), source.size() - prefix.size() - 2);
  while (!payload.empty()) {
    constexpr std::string_view item_prefix{"{\"feature\":\""};
    if (!payload.starts_with(item_prefix) || result.size() >= 256)
      throw std::invalid_argument("target requirement entry is malformed");
    payload.remove_prefix(item_prefix.size());
    const auto feature_end = payload.find('"');
    if (feature_end == std::string_view::npos)
      throw std::invalid_argument("target requirement feature is truncated");
    const auto feature = parse_target_feature(payload.substr(0, feature_end));
    if (!feature)
      throw std::invalid_argument("target requirement feature is unknown");
    payload.remove_prefix(feature_end);
    constexpr std::string_view required_prefix{"\",\"required\":"};
    if (!payload.starts_with(required_prefix))
      throw std::invalid_argument("target requirement flag is malformed");
    payload.remove_prefix(required_prefix.size());
    bool required{};
    if (payload.starts_with("true}")) {
      required = true;
      payload.remove_prefix(5);
    } else if (payload.starts_with("false}")) {
      payload.remove_prefix(6);
    } else {
      throw std::invalid_argument("target requirement flag is invalid");
    }
    result.push_back({*feature, required});
    if (payload.empty())
      break;
    if (!payload.starts_with(','))
      throw std::invalid_argument("target requirement separator is invalid");
    payload.remove_prefix(1);
  }
  if (canonicalize_target_requirements(result) != source)
    throw std::invalid_argument("target requirements are not canonical");
  return result;
}

TargetAdapterDescriptor builtin_target_adapter(std::string_view id) {
  if (id == "portable-package")
    return {
        "portable-package",
        "gspl.target-contract-0.1",
        {capability(TargetFeature::canonical_seed, TargetSupport::native,
                    "verified seed.canonical.json and manifest seed identity"),
         capability(TargetFeature::rights_and_provenance, TargetSupport::native,
                    "verified rights.json, provenance.json, and asset graph"),
         capability(TargetFeature::raster_2d, TargetSupport::native,
                    "checksummed PNG atlas and frame metadata artifacts"),
         capability(TargetFeature::skeletal_2d, TargetSupport::native,
                    "canonical rig and clip artifacts"),
         capability(TargetFeature::animation_graph, TargetSupport::native,
                    "canonical animation graph artifact"),
         capability(TargetFeature::collision_2d, TargetSupport::native,
                    "canonical collision shapes and windows"),
         capability(TargetFeature::channel_maps, TargetSupport::native,
                    "typed checksummed channel-map artifacts")}};
  if (id == "glb-2.0")
    return {"glb-2.0",
            "gspl.target-contract-0.1",
            {capability(TargetFeature::mesh_3d, TargetSupport::native,
                        "validated glTF 2.0 mesh primitives"),
             capability(TargetFeature::pbr_materials_3d, TargetSupport::native,
                        "glTF metallic-roughness materials and textures"),
             capability(TargetFeature::skeleton_3d, TargetSupport::native,
                        "glTF skin, joints, and inverse bind matrices"),
             capability(TargetFeature::morph_targets_3d, TargetSupport::native,
                        "glTF primitive morph targets"),
             capability(TargetFeature::animation_3d, TargetSupport::native,
                        "glTF animation samplers and channels"),
             capability(TargetFeature::lod_3d, TargetSupport::adapter_emulated,
                        "validated LOD meshes with GSPL extras; selection "
                        "remains consumer-owned")}};
  if (id == "godot-4.7-3d") {
    auto adapter = builtin_target_adapter("glb-2.0");
    adapter.id = "godot-4.7-3d";
    adapter.capabilities.push_back(
        capability(TargetFeature::engine_project, TargetSupport::native,
                   "transactional Godot 4.7 project, PackedScene, GLB asset, "
                   "and hash manifest"));
    return adapter;
  }
  throw std::invalid_argument("unknown target adapter: " + std::string(id));
}

} // namespace gspl::sprites
