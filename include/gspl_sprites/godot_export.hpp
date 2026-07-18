#pragma once

#include "gspl_sprites/gltf_export.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <filesystem>
#include <optional>

namespace gspl::sprites {

class TargetSourceEvidence final {
public:
  [[nodiscard]] const std::string &package_identity() const noexcept;
  [[nodiscard]] const std::string &seed_identity() const noexcept;
  [[nodiscard]] const std::string &authoring_provenance_sha256() const noexcept;
  [[nodiscard]] const std::string &target_compatibility_sha256() const noexcept;

private:
  TargetSourceEvidence(std::string package_identity, std::string seed_identity,
                       std::string authoring_provenance_sha256,
                       std::string target_compatibility_sha256);
  std::string package_identity_;
  std::string seed_identity_;
  std::string authoring_provenance_sha256_;
  std::string target_compatibility_sha256_;
  friend TargetSourceEvidence target_source_evidence_from_package(
      const std::filesystem::path &package_root);
};

struct GodotProjectOptions {
  std::string project_name{"GSPL Sprite"};
  GltfExportLimits gltf_limits;
  std::optional<TargetSourceEvidence> source_evidence;
};

struct GodotProjectVerification {
  ValidationResult validation;
  std::string project_identity;
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

[[nodiscard]] TargetSourceEvidence
target_source_evidence_from_package(const std::filesystem::path &package_root);

void export_godot_3d_project(
    const Projection3dDefinition &projection,
    std::span<const AnimationClip3d> animations,
    std::span<const GltfTextureAsset> textures,
    std::span<const TargetRequirement> additional_requirements,
    const std::filesystem::path &output,
    const GodotProjectOptions &options = {});

[[nodiscard]] GodotProjectVerification
verify_godot_3d_project(const std::filesystem::path &root);

} // namespace gspl::sprites
