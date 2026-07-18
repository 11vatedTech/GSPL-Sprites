#include "gspl_sprites/combat.hpp"

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
const CombatAbilityDefinition *ability(const CombatProgram &program,
                                        std::string_view value) {
  const auto found = std::ranges::find(program.abilities, value,
                                       &CombatAbilityDefinition::id);
  return found == program.abilities.end() ? nullptr : &*found;
}
void require_valid(const CombatProgram &program, const CombatState &state) {
  const auto validation = validate_combat_state(program, state);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
}
bool valid_target(const CombatActorState &source, const CombatActorState &target,
                  CombatTargetRule rule) {
  switch (rule) {
  case CombatTargetRule::self: return source.id == target.id;
  case CombatTargetRule::ally: return source.id != target.id && source.team_id == target.team_id;
  case CombatTargetRule::enemy: return source.team_id != target.team_id;
  case CombatTargetRule::any: return true;
  }
  return false;
}
} // namespace

ValidationResult validate_combat_program(const CombatProgram &program) {
  ValidationResult result;
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!id(program.id) || program.maximum_actors == 0 ||
      program.maximum_actors > 100'000 || program.maximum_statuses_per_actor == 0 ||
      program.maximum_statuses_per_actor > 4096 || program.abilities.empty() ||
      program.abilities.size() > 4096)
    add("SPRITE_COMBAT_PROGRAM_INVALID", "combat program identity or bounds are invalid");
  std::set<std::string> abilities;
  for (const auto &value : program.abilities) {
    if (!id(value.id) || !abilities.insert(value.id).second ||
        value.resource_cost > 1'000'000 || value.cooldown_ticks > 86'400'000 ||
        value.maximum_range_millimeters > 10'000'000 || value.effects.empty() ||
        value.effects.size() > 64)
      add("SPRITE_COMBAT_ABILITY_INVALID", "combat ability is malformed, duplicate, or unbounded");
    for (const auto &effect : value.effects) {
      const bool status = effect.kind == CombatEffectKind::status;
      if (effect.magnitude == 0 || effect.magnitude > 1'000'000 ||
          (status && (!id(effect.status_id) || effect.duration_ticks == 0 ||
                      effect.duration_ticks > 86'400'000)) ||
          (!status && (!effect.status_id.empty() || effect.duration_ticks != 0)))
        add("SPRITE_COMBAT_EFFECT_INVALID", "combat effect semantics are invalid");
    }
  }
  return result;
}

ValidationResult validate_combat_state(const CombatProgram &program,
                                       const CombatState &state) {
  auto result = validate_combat_program(program);
  if (!result.ok()) return result;
  if (state.actors.empty() || state.actors.size() > program.maximum_actors) {
    result.diagnostics.push_back({"SPRITE_COMBAT_STATE_INVALID", "combat actor bound is invalid"});
    return result;
  }
  for (const auto &[key, actor] : state.actors) {
    if (key != actor.id || !id(actor.id) || !id(actor.team_id) ||
        actor.maximum_health == 0 || actor.maximum_health > 1'000'000 ||
        actor.health > actor.maximum_health || actor.maximum_resource > 1'000'000 ||
        actor.resource > actor.maximum_resource || actor.x_millimeters < -10'000'000 ||
        actor.x_millimeters > 10'000'000 || actor.y_millimeters < -10'000'000 ||
        actor.y_millimeters > 10'000'000 ||
        actor.cooldown_expiry.size() > program.abilities.size() ||
        actor.statuses.size() > program.maximum_statuses_per_actor) {
      result.diagnostics.push_back({"SPRITE_COMBAT_ACTOR_INVALID", "combat actor state is invalid"});
      continue;
    }
    for (const auto &[ability_id, expiry] : actor.cooldown_expiry)
      if (ability(program, ability_id) == nullptr || expiry <= state.tick)
        result.diagnostics.push_back({"SPRITE_COMBAT_COOLDOWN_INVALID", "combat cooldown is invalid"});
    for (const auto &[status_id, status] : actor.statuses)
      if (!id(status_id) || status.magnitude == 0 || status.magnitude > 1'000'000 ||
          status.expires_tick <= state.tick)
        result.diagnostics.push_back({"SPRITE_COMBAT_STATUS_INVALID", "combat status is invalid"});
  }
  return result;
}

void advance_combat_to(const CombatProgram &program, CombatState &state,
                       std::uint64_t tick) {
  require_valid(program, state);
  if (tick < state.tick) throw std::invalid_argument("combat time cannot roll back");
  auto candidate = state;
  candidate.tick = tick;
  for (auto &[key, actor] : candidate.actors) {
    static_cast<void>(key);
    std::erase_if(actor.cooldown_expiry, [&](const auto &entry) { return entry.second <= tick; });
    std::erase_if(actor.statuses, [&](const auto &entry) { return entry.second.expires_tick <= tick; });
  }
  require_valid(program, candidate);
  state = std::move(candidate);
}

std::vector<CombatEvent> execute_combat_command(const CombatProgram &program,
                                                CombatState &state,
                                                const CombatCommand &command) {
  require_valid(program, state);
  if (command.tick < state.tick) throw std::invalid_argument("combat command is stale");
  auto candidate = state;
  advance_combat_to(program, candidate, command.tick);
  auto source_it = candidate.actors.find(command.source_actor);
  auto target_it = candidate.actors.find(command.target_actor);
  const auto *definition = ability(program, command.ability_id);
  if (source_it == candidate.actors.end() || target_it == candidate.actors.end() ||
      definition == nullptr) throw std::invalid_argument("combat command references absent semantics");
  auto &source = source_it->second;
  auto &target = target_it->second;
  if (source.health == 0 || target.health == 0 ||
      !valid_target(source, target, definition->target_rule) ||
      source.resource < definition->resource_cost ||
      source.cooldown_expiry.contains(definition->id))
    throw std::runtime_error("combat ability activation requirements failed");
  const auto dx = static_cast<std::int64_t>(source.x_millimeters) - target.x_millimeters;
  const auto dy = static_cast<std::int64_t>(source.y_millimeters) - target.y_millimeters;
  const auto range = static_cast<std::int64_t>(definition->maximum_range_millimeters);
  if (dx * dx + dy * dy > range * range)
    throw std::runtime_error("combat target is out of range");
  source.resource -= definition->resource_cost;
  if (definition->cooldown_ticks > 0) {
    if (command.tick > std::numeric_limits<std::uint64_t>::max() - definition->cooldown_ticks)
      throw std::overflow_error("combat cooldown tick overflow");
    source.cooldown_expiry[definition->id] = command.tick + definition->cooldown_ticks;
  }
  std::vector<CombatEvent> events;
  for (const auto &effect : definition->effects) {
    std::uint32_t applied{};
    if (effect.kind == CombatEffectKind::damage) {
      applied = std::min(target.health, effect.magnitude);
      target.health -= applied;
    } else if (effect.kind == CombatEffectKind::healing) {
      applied = std::min(effect.magnitude, target.maximum_health - target.health);
      target.health += applied;
    } else {
      if (!target.statuses.contains(effect.status_id) &&
          target.statuses.size() >= program.maximum_statuses_per_actor)
        throw std::runtime_error("combat status bound exceeded");
      if (command.tick > std::numeric_limits<std::uint64_t>::max() - effect.duration_ticks)
        throw std::overflow_error("combat status tick overflow");
      target.statuses[effect.status_id] = {effect.magnitude, command.tick + effect.duration_ticks};
      applied = effect.magnitude;
    }
    if (candidate.next_sequence == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("combat event sequence overflow");
    events.push_back({command.tick, candidate.next_sequence++, source.id, target.id,
                      definition->id, effect.kind, effect.status_id, applied});
  }
  require_valid(program, candidate);
  state = std::move(candidate);
  return events;
}

} // namespace gspl::sprites
