#pragma once

#include "gspl_sprites/gltf_export.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <filesystem>

namespace gspl::sprites {

struct GodotProjectOptions {
  std::string project_name{"GSPL Sprite"};
  GltfExportLimits gltf_limits;
};

struct GodotProjectVerification {
  ValidationResult validation;
  std::string project_identity;
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

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
