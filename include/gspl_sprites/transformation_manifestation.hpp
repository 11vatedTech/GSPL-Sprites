#pragma once

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection2d.hpp"
#include "gspl_sprites/transformation_persistence.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gspl::sprites {

struct FormManifestationBinding {
  std::string form_id;
  std::string animation_state_id;
  std::string projection_id;
};
struct TransitionManifestationBinding {
  std::string transformation_id;
  std::string animation_clip_id;
};
struct TransformationManifestationProgram {
  std::string id;
  std::vector<FormManifestationBinding> forms;
  std::vector<TransitionManifestationBinding> transitions;
};
struct TransformationManifestationFrame {
  std::string stable_entity_id;
  std::string authoritative_state_identity;
  std::string form_id;
  std::string animation_state_id;
  std::string projection_id;
  std::optional<std::string> transition_animation_clip_id;
  std::uint32_t transition_progress_per_million{};
};

[[nodiscard]] ValidationResult validate_transformation_manifestation_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection3dDefinition> projections);
[[nodiscard]] TransformationManifestationFrame
project_transformation_manifestation(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection3dDefinition> projections,
    const TransformationState &state, std::uint64_t tick);

[[nodiscard]] ValidationResult validate_transformation_manifestation25d_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection25dDefinition> projections);
[[nodiscard]] TransformationManifestationFrame
project_transformation_manifestation25d(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection25dDefinition> projections,
    const TransformationState &state, std::uint64_t tick);

[[nodiscard]] ValidationResult validate_transformation_manifestation2d_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection2dDefinition> projections);
[[nodiscard]] TransformationManifestationFrame
project_transformation_manifestation2d(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection2dDefinition> projections,
    const TransformationState &state, std::uint64_t tick);

} // namespace gspl::sprites
