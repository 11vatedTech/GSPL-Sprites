#include "gspl_sprites/transformation_manifestation.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

namespace gspl::sprites {
namespace {
bool id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
         });
}
template <class Range, class Projection>
bool contains(const Range &range, std::string_view value, Projection projection) {
  return std::ranges::find(range, value, projection) != std::ranges::end(range);
}
} // namespace

ValidationResult validate_transformation_manifestation_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection3dDefinition> projections) {
  ValidationResult result;
  const auto transformation_validation =
      validate_transformation_program(transformation_program, combat_program);
  result.diagnostics.insert(result.diagnostics.end(), transformation_validation.diagnostics.begin(),
                            transformation_validation.diagnostics.end());
  const auto graph_validation = validate_state_graph(animation_graph, clips);
  result.diagnostics.insert(result.diagnostics.end(), graph_validation.diagnostics.begin(),
                            graph_validation.diagnostics.end());
  for (const auto &projection : projections) {
    const auto validation = validate_projection3d(projection);
    result.diagnostics.insert(result.diagnostics.end(), validation.diagnostics.begin(),
                              validation.diagnostics.end());
  }
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!id(program.id) || program.forms.size() != transformation_program.forms.size() ||
      program.transitions.size() != transformation_program.transformations.size() ||
      projections.empty())
    add("SPRITE_MANIFESTATION_PROGRAM_INVALID", "manifestation identity or coverage is invalid");
  std::set<std::string> forms;
  for (const auto &binding : program.forms) {
    if (!forms.insert(binding.form_id).second ||
        !contains(transformation_program.forms, binding.form_id, &TransformationFormDefinition::id) ||
        !contains(animation_graph.states, binding.animation_state_id, &AnimationState::id) ||
        !contains(projections, binding.projection_id, &Projection3dDefinition::id))
      add("SPRITE_FORM_MANIFESTATION_INVALID", "form manifestation is absent, duplicate, or unresolved");
  }
  std::set<std::string> transitions;
  for (const auto &binding : program.transitions) {
    if (!transitions.insert(binding.transformation_id).second ||
        !contains(transformation_program.transformations, binding.transformation_id,
                  &TransformationDefinition::id) ||
        !contains(clips, binding.animation_clip_id, &SkeletalClip::id))
      add("SPRITE_TRANSITION_MANIFESTATION_INVALID", "transition manifestation is absent, duplicate, or unresolved");
  }
  return result;
}

TransformationManifestationFrame project_transformation_manifestation(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection3dDefinition> projections,
    const TransformationState &state, std::uint64_t tick) {
  const auto validation = validate_transformation_manifestation_program(
      program, transformation_program, combat_program, animation_graph, clips, projections);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  const auto state_validation = validate_transformation_state(transformation_program, combat_program, state);
  if (!state_validation.ok()) throw std::invalid_argument(state_validation.diagnostics.front().message);
  const auto form = std::ranges::find(program.forms, state.current_form,
                                      &FormManifestationBinding::form_id);
  if (form == program.forms.end()) throw std::logic_error("validated form manifestation is absent");
  TransformationManifestationFrame frame{state.stable_entity_id,
      transformation_state_identity(transformation_program, combat_program, state),
      state.current_form, form->animation_state_id, form->projection_id};
  if (state.active) {
    if (tick < state.active->started_tick || tick > state.active->completes_tick)
      throw std::invalid_argument("manifestation tick is outside active transformation");
    const auto binding = std::ranges::find(program.transitions,
        state.active->transformation_id, &TransitionManifestationBinding::transformation_id);
    frame.transition_animation_clip_id = binding->animation_clip_id;
    const auto elapsed = tick - state.active->started_tick;
    const auto duration = state.active->completes_tick - state.active->started_tick;
    frame.transition_progress_per_million = static_cast<std::uint32_t>(
        elapsed * 1'000'000ULL / duration);
  }
  return frame;
}

ValidationResult validate_transformation_manifestation25d_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection25dDefinition> projections) {
  ValidationResult result;
  const auto transformation_validation =
      validate_transformation_program(transformation_program, combat_program);
  result.diagnostics.insert(result.diagnostics.end(), transformation_validation.diagnostics.begin(),
                            transformation_validation.diagnostics.end());
  const auto graph_validation = validate_state_graph(animation_graph, clips);
  result.diagnostics.insert(result.diagnostics.end(), graph_validation.diagnostics.begin(),
                            graph_validation.diagnostics.end());
  for (const auto &projection : projections) {
    const auto validation = validate_projection25d(projection);
    result.diagnostics.insert(result.diagnostics.end(), validation.diagnostics.begin(),
                              validation.diagnostics.end());
  }
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!id(program.id) || program.forms.size() != transformation_program.forms.size() ||
      program.transitions.size() != transformation_program.transformations.size() ||
      projections.empty())
    add("SPRITE_MANIFESTATION_25D_PROGRAM_INVALID", "2.5D manifestation identity or coverage is invalid");
  std::set<std::string> forms;
  for (const auto &binding : program.forms) {
    if (!forms.insert(binding.form_id).second ||
        !contains(transformation_program.forms, binding.form_id, &TransformationFormDefinition::id) ||
        !contains(animation_graph.states, binding.animation_state_id, &AnimationState::id) ||
        !contains(projections, binding.projection_id, &Projection25dDefinition::id))
      add("SPRITE_FORM_MANIFESTATION_25D_INVALID", "2.5D form manifestation is unresolved or duplicate");
  }
  std::set<std::string> transitions;
  for (const auto &binding : program.transitions) {
    if (!transitions.insert(binding.transformation_id).second ||
        !contains(transformation_program.transformations, binding.transformation_id,
                  &TransformationDefinition::id) ||
        !contains(clips, binding.animation_clip_id, &SkeletalClip::id))
      add("SPRITE_TRANSITION_MANIFESTATION_25D_INVALID", "2.5D transition manifestation is unresolved or duplicate");
  }
  return result;
}

TransformationManifestationFrame project_transformation_manifestation25d(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection25dDefinition> projections,
    const TransformationState &state, std::uint64_t tick) {
  const auto validation = validate_transformation_manifestation25d_program(
      program, transformation_program, combat_program, animation_graph, clips, projections);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  const auto state_validation = validate_transformation_state(transformation_program, combat_program, state);
  if (!state_validation.ok()) throw std::invalid_argument(state_validation.diagnostics.front().message);
  const auto form = std::ranges::find(program.forms, state.current_form,
                                      &FormManifestationBinding::form_id);
  TransformationManifestationFrame frame{state.stable_entity_id,
      transformation_state_identity(transformation_program, combat_program, state),
      state.current_form, form->animation_state_id, form->projection_id};
  if (state.active) {
    if (tick < state.active->started_tick || tick > state.active->completes_tick)
      throw std::invalid_argument("2.5D manifestation tick is outside active transformation");
    const auto binding = std::ranges::find(program.transitions,
        state.active->transformation_id, &TransitionManifestationBinding::transformation_id);
    frame.transition_animation_clip_id = binding->animation_clip_id;
    const auto elapsed = tick - state.active->started_tick;
    const auto duration = state.active->completes_tick - state.active->started_tick;
    frame.transition_progress_per_million = static_cast<std::uint32_t>(
        elapsed * 1'000'000ULL / duration);
  }
  return frame;
}

ValidationResult validate_transformation_manifestation2d_program(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection2dDefinition> projections) {
  ValidationResult result;
  const auto transformation_validation =
      validate_transformation_program(transformation_program, combat_program);
  result.diagnostics.insert(result.diagnostics.end(), transformation_validation.diagnostics.begin(),
                            transformation_validation.diagnostics.end());
  const auto graph_validation = validate_state_graph(animation_graph, clips);
  result.diagnostics.insert(result.diagnostics.end(), graph_validation.diagnostics.begin(),
                            graph_validation.diagnostics.end());
  for (const auto &projection : projections) {
    const auto validation = validate_projection2d(projection);
    result.diagnostics.insert(result.diagnostics.end(), validation.diagnostics.begin(),
                              validation.diagnostics.end());
  }
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!id(program.id) || program.forms.size() != transformation_program.forms.size() ||
      program.transitions.size() != transformation_program.transformations.size() ||
      projections.empty())
    add("SPRITE_MANIFESTATION_2D_PROGRAM_INVALID", "2D manifestation identity or coverage is invalid");
  std::set<std::string> forms;
  for (const auto &binding : program.forms) {
    if (!forms.insert(binding.form_id).second ||
        !contains(transformation_program.forms, binding.form_id, &TransformationFormDefinition::id) ||
        !contains(animation_graph.states, binding.animation_state_id, &AnimationState::id) ||
        !contains(projections, binding.projection_id, &Projection2dDefinition::id))
      add("SPRITE_FORM_MANIFESTATION_2D_INVALID", "2D form manifestation is unresolved or duplicate");
  }
  std::set<std::string> transitions;
  for (const auto &binding : program.transitions) {
    if (!transitions.insert(binding.transformation_id).second ||
        !contains(transformation_program.transformations, binding.transformation_id,
                  &TransformationDefinition::id) ||
        !contains(clips, binding.animation_clip_id, &SkeletalClip::id))
      add("SPRITE_TRANSITION_MANIFESTATION_2D_INVALID", "2D transition manifestation is unresolved or duplicate");
  }
  return result;
}

TransformationManifestationFrame project_transformation_manifestation2d(
    const TransformationManifestationProgram &program,
    const TransformationProgram &transformation_program,
    const CombatProgram &combat_program, const AnimationStateGraph &animation_graph,
    std::span<const SkeletalClip> clips,
    std::span<const Projection2dDefinition> projections,
    const TransformationState &state, std::uint64_t tick) {
  const auto validation = validate_transformation_manifestation2d_program(
      program, transformation_program, combat_program, animation_graph, clips, projections);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  const auto state_validation = validate_transformation_state(transformation_program, combat_program, state);
  if (!state_validation.ok()) throw std::invalid_argument(state_validation.diagnostics.front().message);
  const auto form = std::ranges::find(program.forms, state.current_form,
                                      &FormManifestationBinding::form_id);
  TransformationManifestationFrame frame{state.stable_entity_id,
      transformation_state_identity(transformation_program, combat_program, state),
      state.current_form, form->animation_state_id, form->projection_id};
  if (state.active) {
    if (tick < state.active->started_tick || tick > state.active->completes_tick)
      throw std::invalid_argument("2D manifestation tick is outside active transformation");
    const auto binding = std::ranges::find(program.transitions,
        state.active->transformation_id, &TransitionManifestationBinding::transformation_id);
    frame.transition_animation_clip_id = binding->animation_clip_id;
    const auto elapsed = tick - state.active->started_tick;
    const auto duration = state.active->completes_tick - state.active->started_tick;
    frame.transition_progress_per_million = static_cast<std::uint32_t>(
        elapsed * 1'000'000ULL / duration);
  }
  return frame;
}

} // namespace gspl::sprites
