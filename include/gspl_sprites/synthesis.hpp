#pragma once

#include <cstdint>

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/projection2d.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/animation3d.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include <string>
#include <vector>

namespace gspl::sprites {

struct SynthesisPalette {
  std::uint32_t primary;
  std::uint32_t secondary;
  std::uint32_t accent;
  std::uint32_t outline;
  std::uint32_t background;
};

[[nodiscard]] SynthesisPalette make_palette(std::string_view primary_hex, std::string_view accent_hex);
[[nodiscard]] RigDefinition make_biped_rig(std::string_view id);

[[nodiscard]] Projection2dDefinition synthesize_projection2d(std::string_view entity_id, std::string_view form_id,
                                                             const SynthesisPalette& palette,
                                                             const RigDefinition& rig);

[[nodiscard]] Projection25dDefinition synthesize_projection25d(std::string_view entity_id, std::string_view form_id,
                                                                const SynthesisPalette& palette,
                                                                const RigDefinition& rig);

[[nodiscard]] Projection3dDefinition synthesize_projection3d(std::string_view entity_id, std::string_view form_id,
                                                               const SynthesisPalette& palette,
                                                               const RigDefinition& rig);

[[nodiscard]] Projection3dDefinition synthesize_projection3d_voltfox(std::string_view entity_id, std::string_view form_id,
                                                                        const SynthesisPalette& palette,
                                                                        const std::map<std::string, MorphologyPart, std::less<>>& morphology);

[[nodiscard]] Projection25dDefinition synthesize_projection25d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology);

[[nodiscard]] Projection2dDefinition synthesize_projection2d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const RigDefinition& rig);

[[nodiscard]] std::vector<AnimationClip3d> synthesize_animation3d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const std::vector<SkeletalClip>& clips,
    std::span<const AnimationIntent> animation_intents);

[[nodiscard]] TransformationManifestationProgram make_manifestation2d(std::string_view entity_id,
                                                                       const RigDefinition& rig);
[[nodiscard]] TransformationManifestationProgram make_manifestation25d(std::string_view entity_id,
                                                                        const RigDefinition& rig);
[[nodiscard]] TransformationManifestationProgram make_manifestation3d(std::string_view entity_id,
                                                                       const RigDefinition& rig);

struct SynthesisResult {
  Projection2dDefinition proj2d_base;
  Projection2dDefinition proj2d_transformed;
  Projection25dDefinition proj25d_base;
  Projection25dDefinition proj25d_transformed;
  Projection3dDefinition proj3d_base;
  Projection3dDefinition proj3d_transformed;
  std::vector<AnimationClip3d> animations3d;
  TransformationManifestationProgram manifest2d;
  TransformationManifestationProgram manifest25d;
  TransformationManifestationProgram manifest3d;
};

[[nodiscard]] SynthesisResult synthesize_unified_entity(std::string_view entity_id,
                                                        const SynthesisPalette& base_palette,
                                                        const SynthesisPalette& transformed_palette);

[[nodiscard]] SynthesisResult synthesize_unified_entity(const SpriteIr& ir);

[[nodiscard]] ValidationResult enforce_resource_limits(const SpriteSeed& seed,
                                                         const SynthesisResult& result,
                                                         const ResourceLimits& limits = {});

} // namespace gspl::sprites