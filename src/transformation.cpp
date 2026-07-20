#include "gspl_sprites/transformation.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
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
const TransformationFormDefinition *form(const TransformationProgram &program,
                                          std::string_view value) {
  const auto found = std::ranges::find(program.forms, value,
                                       &TransformationFormDefinition::id);
  return found == program.forms.end() ? nullptr : &*found;
}
const TransformationDefinition *transformation(
    const TransformationProgram &program, std::string_view value) {
  const auto found = std::ranges::find(program.transformations, value,
                                       &TransformationDefinition::id);
  return found == program.transformations.end() ? nullptr : &*found;
}
bool ability(const CombatProgram &program, std::string_view value) {
  return std::ranges::find(program.abilities, value,
                           &CombatAbilityDefinition::id) != program.abilities.end();
}
void require_valid(const TransformationProgram &program,
                   const CombatProgram &combat,
                   const TransformationState &state) {
  const auto validation = validate_transformation_state(program, combat, state);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
}
std::uint32_t apply_delta(std::uint32_t current, std::int32_t old_delta,
                          std::int32_t new_delta, std::string_view field) {
  const auto base = static_cast<std::int64_t>(current) - old_delta;
  const auto result = base + new_delta;
  if (base <= 0 || result <= 0 || result > 1'000'000)
    throw std::runtime_error("transformation " + std::string(field) +
                             " delta violates actor bounds");
  return static_cast<std::uint32_t>(result);
}
} // namespace

ValidationResult validate_transformation_program(
    const TransformationProgram &program, const CombatProgram &combat_program) {
  ValidationResult result;
  const auto combat_validation = validate_combat_program(combat_program);
  result.diagnostics.insert(result.diagnostics.end(), combat_validation.diagnostics.begin(),
                            combat_validation.diagnostics.end());
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!combat_validation.ok()) return result;
  if (!id(program.id) || program.forms.empty() || program.forms.size() > 1024 ||
      program.transformations.size() > 4096 || program.maximum_history == 0 ||
      program.maximum_history > 4096)
    add("SPRITE_TRANSFORMATION_PROGRAM_INVALID", "transformation identity or bounds are invalid");
  std::set<std::string> forms;
  for (const auto &value : program.forms) {
    std::set<std::string> abilities;
    if (!id(value.id) || !forms.insert(value.id).second ||
        value.maximum_health_delta < -999'999 || value.maximum_health_delta > 999'999 ||
        value.maximum_resource_delta < -999'999 || value.maximum_resource_delta > 999'999 ||
        value.enabled_abilities.empty() || value.enabled_abilities.size() > combat_program.abilities.size())
      add("SPRITE_TRANSFORMATION_FORM_INVALID", "transformation form is malformed or duplicate");
    for (const auto &ability_id : value.enabled_abilities)
      if (!ability(combat_program, ability_id) || !abilities.insert(ability_id).second)
        add("SPRITE_TRANSFORMATION_ABILITY_INVALID", "form ability is absent or duplicate");
  }
  if (!forms.contains(program.base_form))
    add("SPRITE_TRANSFORMATION_BASE_INVALID", "base transformation form is absent");
  std::set<std::string> transformations;
  std::set<std::pair<std::string, std::string>> edges;
  for (const auto &value : program.transformations) {
    if (!id(value.id) || !transformations.insert(value.id).second ||
        !forms.contains(value.from_form) || !forms.contains(value.to_form) ||
        value.from_form == value.to_form || value.energy_cost > 1'000'000 ||
        value.duration_ticks == 0 || value.duration_ticks > 3'600'000 ||
        !edges.emplace(value.from_form, value.to_form).second)
      add("SPRITE_TRANSFORMATION_INVALID", "transformation is malformed, duplicate, or unbounded");
    if (value.reversible &&
        std::ranges::none_of(program.transformations, [&](const auto &reverse) {
          return reverse.from_form == value.to_form && reverse.to_form == value.from_form;
        }))
      add("SPRITE_TRANSFORMATION_REVERSE_MISSING", "reversible transformation lacks an explicit reverse");
  }
  std::set<std::string> reachable{program.base_form};
  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto &value : program.transformations)
      if (reachable.contains(value.from_form) && reachable.insert(value.to_form).second)
        changed = true;
  }
  for (const auto &value : program.forms)
    if (!reachable.contains(value.id))
      add("SPRITE_TRANSFORMATION_FORM_UNREACHABLE", "form is unreachable from the base form");
  return result;
}

ValidationResult validate_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state) {
  auto result = validate_transformation_program(program, combat_program);
  if (!result.ok()) return result;
  if (!id(state.stable_entity_id) || form(program, state.current_form) == nullptr ||
      state.maximum_energy > 1'000'000 || state.energy > state.maximum_energy ||
      state.history.size() > program.maximum_history)
    result.diagnostics.push_back({"SPRITE_TRANSFORMATION_STATE_INVALID", "transformation state is invalid"});
  std::string expected = program.base_form;
  std::uint64_t previous{};
  for (const auto &entry : state.history) {
    const auto *definition = transformation(program, entry.transformation_id);
    if (definition == nullptr || entry.from_form != expected ||
        definition->from_form != entry.from_form || definition->to_form != entry.to_form ||
        entry.completed_tick < previous)
      result.diagnostics.push_back({"SPRITE_TRANSFORMATION_HISTORY_INVALID", "transformation history is inconsistent"});
    expected = entry.to_form;
    previous = entry.completed_tick;
  }
  if (expected != state.current_form)
    result.diagnostics.push_back({"SPRITE_TRANSFORMATION_FORM_STATE_INVALID", "current form differs from history"});
  if (state.active) {
    const auto *definition = transformation(program, state.active->transformation_id);
    if (definition == nullptr || state.active->from_form != state.current_form ||
        definition->from_form != state.active->from_form || definition->to_form != state.active->to_form ||
        state.active->completes_tick <= state.active->started_tick ||
        state.active->completes_tick - state.active->started_tick != definition->duration_ticks)
      result.diagnostics.push_back({"SPRITE_TRANSFORMATION_ACTIVE_INVALID", "active transformation is inconsistent"});
  }
  return result;
}

void begin_transformation(const TransformationProgram &program,
                          const CombatProgram &combat_program,
                          TransformationState &state,
                          std::string_view transformation_id,
                          std::uint64_t tick) {
  require_valid(program, combat_program, state);
  const auto *definition = transformation(program, transformation_id);
  if (definition == nullptr || state.active || definition->from_form != state.current_form ||
      state.energy < definition->energy_cost)
    throw std::runtime_error("transformation activation requirements failed");
  if (tick > std::numeric_limits<std::uint64_t>::max() - definition->duration_ticks)
    throw std::overflow_error("transformation completion tick overflow");
  auto candidate = state;
  candidate.energy -= definition->energy_cost;
  candidate.active = ActiveTransformation{definition->id, definition->from_form,
                                           definition->to_form, tick,
                                           tick + definition->duration_ticks};
  require_valid(program, combat_program, candidate);
  state = std::move(candidate);
}

void advance_transformation_to(const TransformationProgram &program,
                               const CombatProgram &combat_program,
                               TransformationState &state,
                               CombatState &combat_state, std::uint64_t tick) {
  require_valid(program, combat_program, state);
  const auto combat_validation = validate_combat_state(combat_program, combat_state);
  if (!combat_validation.ok()) throw std::invalid_argument(combat_validation.diagnostics.front().message);
  auto state_candidate = state;
  auto combat_candidate = combat_state;
  if (!state_candidate.active || tick < state_candidate.active->completes_tick) return;
  if (state_candidate.history.size() >= program.maximum_history)
    throw std::runtime_error("transformation history bound exceeded");
  auto actor = combat_candidate.actors.find(state_candidate.stable_entity_id);
  if (actor == combat_candidate.actors.end())
    throw std::runtime_error("transformation entity is absent from combat state");
  const auto *old_form = form(program, state_candidate.active->from_form);
  const auto *new_form = form(program, state_candidate.active->to_form);
  actor->second.maximum_health = apply_delta(actor->second.maximum_health,
      old_form->maximum_health_delta, new_form->maximum_health_delta, "health");
  actor->second.maximum_resource = apply_delta(actor->second.maximum_resource,
      old_form->maximum_resource_delta, new_form->maximum_resource_delta, "resource");
  actor->second.health = std::min(actor->second.health, actor->second.maximum_health);
  actor->second.resource = std::min(actor->second.resource, actor->second.maximum_resource);
  state_candidate.history.push_back({state_candidate.active->transformation_id,
      state_candidate.active->from_form, state_candidate.active->to_form,
      state_candidate.active->completes_tick});
  state_candidate.current_form = state_candidate.active->to_form;
  state_candidate.active.reset();
  require_valid(program, combat_program, state_candidate);
  const auto updated_combat = validate_combat_state(combat_program, combat_candidate);
  if (!updated_combat.ok()) throw std::runtime_error(updated_combat.diagnostics.front().message);
  state = std::move(state_candidate);
  combat_state = std::move(combat_candidate);
}

std::vector<CombatEvent> execute_transformed_combat_command(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &transformation_state, CombatState &combat_state,
    const CombatCommand &command) {
  require_valid(program, combat_program, transformation_state);
  const auto *current = form(program, transformation_state.current_form);
  if (transformation_state.active || command.source_actor != transformation_state.stable_entity_id ||
      !std::ranges::contains(current->enabled_abilities, command.ability_id))
    throw std::runtime_error("ability is unavailable in the current transformation form");
  return execute_combat_command(combat_program, combat_state, command);
}

TransformationVfxEvent
trigger_transformation_vfx(const TransformationProgram &program,
                           std::string_view transformation_id,
                           std::uint64_t tick) {
  const auto *definition = transformation(program, transformation_id);
  if (definition == nullptr)
    throw std::runtime_error("transformation not found for VFX trigger");
  TransformationVfxEvent result;
  result.transformation_id = definition->id;
  result.from_form = definition->from_form;
  result.to_form = definition->to_form;
  result.tick = tick;
  result.duration_ticks = definition->duration_ticks * 5;
  return result;
}

} // namespace gspl::sprites
