#pragma once

#include "gspl_sprites/common.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace gspl::sprites {

enum class TargetFeature {
  canonical_seed,
  rights_and_provenance,
  raster_2d,
  skeletal_2d,
  animation_graph,
  collision_2d,
  channel_maps,
  living_runtime,
  deterministic_replay,
  depth_planes_25d,
  multi_angle_25d,
  hybrid_geometry_25d,
  mesh_3d,
  pbr_materials_3d,
  skeleton_3d,
  morph_targets_3d,
  animation_3d,
  lod_3d,
  engine_project
};

enum class TargetSupport { native, adapter_emulated, unsupported };

struct TargetCapability {
  TargetFeature feature{};
  TargetSupport support{TargetSupport::unsupported};
  std::string evidence;
};

struct TargetAdapterDescriptor {
  std::string id;
  std::string contract_version;
  std::vector<TargetCapability> capabilities;
};

struct TargetRequirement {
  TargetFeature feature{};
  bool required{true};
};

struct TargetFeatureResolution {
  TargetFeature feature{};
  bool required{};
  TargetSupport support{TargetSupport::unsupported};
  std::string evidence;
};

struct TargetCompatibilityReport {
  std::string adapter_id;
  std::string contract_version;
  std::vector<TargetFeatureResolution> resolutions;
  ValidationResult validation;
  [[nodiscard]] bool compatible() const noexcept { return validation.ok(); }
};

[[nodiscard]] std::string_view
target_feature_name(TargetFeature feature) noexcept;
[[nodiscard]] std::optional<TargetFeature>
parse_target_feature(std::string_view name) noexcept;
[[nodiscard]] ValidationResult
validate_target_adapter(const TargetAdapterDescriptor &adapter);
[[nodiscard]] TargetCompatibilityReport
evaluate_target_compatibility(const TargetAdapterDescriptor &adapter,
                              std::span<const TargetRequirement> requirements);
[[nodiscard]] std::string
canonicalize_target_compatibility(const TargetCompatibilityReport &report);
[[nodiscard]] TargetAdapterDescriptor
builtin_target_adapter(std::string_view id);

} // namespace gspl::sprites
