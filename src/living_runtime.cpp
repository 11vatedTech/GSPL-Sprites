#include "gspl_sprites/living_runtime.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
bool stable_id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
         });
}
bool compare(std::int32_t value, Comparison comparison,
             std::int32_t threshold) {
  switch (comparison) {
  case Comparison::equal:
    return value == threshold;
  case Comparison::not_equal:
    return value != threshold;
  case Comparison::less:
    return value < threshold;
  case Comparison::less_equal:
    return value <= threshold;
  case Comparison::greater:
    return value > threshold;
  case Comparison::greater_equal:
    return value >= threshold;
  }
  throw std::logic_error("unreachable runtime comparison");
}
std::int32_t variable(const LivingRuntimeState &state, std::string_view key) {
  const auto found = state.variables.find(key);
  return found == state.variables.end() ? 0 : found->second;
}
bool conditions(std::span<const RuntimeCondition> values,
                const LivingRuntimeState &state) {
  return std::ranges::all_of(values, [&](const auto &condition) {
    return compare(variable(state, condition.variable), condition.comparison,
                   condition.threshold);
  });
}
const RuntimeActionDefinition *action(const LivingRuntimeProgram &program,
                                      std::string_view id) {
  const auto found =
      std::ranges::find(program.actions, id, &RuntimeActionDefinition::id);
  return found == program.actions.end() ? nullptr : &*found;
}
RuntimeEvent event(LivingRuntimeState &state, RuntimeEventKind kind,
                   std::string action_id, std::string marker = {}) {
  return {state.tick, state.next_sequence++, kind, std::move(action_id),
          std::move(marker)};
}
void refresh_perception(LivingRuntimeState &state, std::string_view key) {
  const RuntimeMemoryRecord *latest = nullptr;
  for (const auto &record : state.memory)
    if (record.observation.key == key &&
        (latest == nullptr ||
         std::tuple{record.observation.observed_tick, record.sequence} >
             std::tuple{latest->observation.observed_tick, latest->sequence}))
      latest = &record;
  if (latest) {
    state.variables.insert_or_assign("perception." + std::string(key),
                                     latest->observation.value);
    state.variables.insert_or_assign(
        "confidence." + std::string(key),
        static_cast<std::int32_t>(latest->observation.confidence_per_million));
  } else {
    state.variables.erase("perception." + std::string(key));
    state.variables.erase("confidence." + std::string(key));
  }
}
void require_state(const LivingRuntimeProgram &program,
                   const LivingRuntimeState &state) {
  if (state.energy > 1'000'000 || state.variables.size() > 4096 ||
      state.memory.size() > program.maximum_memory_records ||
      state.cooldowns.size() > program.actions.size())
    throw std::invalid_argument(
        "living runtime state exceeds structural bounds");
  for (const auto &[key, value] : state.variables) {
    static_cast<void>(value);
    if (!stable_id(key))
      throw std::invalid_argument(
          "living runtime state contains an invalid variable");
  }
  std::set<std::uint64_t> memory_sequences;
  for (const auto &record : state.memory)
    if (!stable_id(record.observation.key) ||
        !stable_id(record.observation.source_entity) ||
        record.observation.confidence_per_million > 1'000'000 ||
        record.observation.lifetime_ticks == 0 ||
        record.observation.observed_tick > state.tick ||
        record.sequence >= state.next_sequence ||
        !memory_sequences.insert(record.sequence).second)
      throw std::invalid_argument("living runtime memory is invalid");
  for (const auto &[id, remaining] : state.cooldowns) {
    const auto *definition = action(program, id);
    if (definition == nullptr || remaining == 0 ||
        remaining > definition->cooldown_ticks)
      throw std::invalid_argument("living runtime cooldown state is invalid");
  }
  if (state.active_action) {
    const auto *definition = action(program, state.active_action->action_id);
    if (definition == nullptr ||
        state.active_action->started_tick > state.tick ||
        state.active_action->remaining_ticks == 0 ||
        state.active_action->remaining_ticks > definition->duration_ticks ||
        state.active_action->elapsed_ticks >= definition->duration_ticks ||
        state.active_action->elapsed_ticks +
                state.active_action->remaining_ticks !=
            definition->duration_ticks)
      throw std::invalid_argument("living runtime active action is invalid");
  }
}
} // namespace

ValidationResult
validate_living_runtime_program(const LivingRuntimeProgram &program) {
  ValidationResult result;
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!stable_id(program.id) || program.ticks_per_second == 0 ||
      program.ticks_per_second > 1000 || program.maximum_memory_records == 0 ||
      program.maximum_memory_records > 1'000'000)
    add("SPRITE_RUNTIME_PROGRAM_INVALID",
        "runtime identity, tick rate, or memory bound is invalid");
  if (program.goals.empty() || program.goals.size() > 1024 ||
      program.actions.empty() || program.actions.size() > 4096)
    add("SPRITE_RUNTIME_CONTENT_INVALID",
        "runtime requires bounded nonempty goals and actions");
  std::set<std::string> goals;
  for (const auto &goal : program.goals) {
    if (!stable_id(goal.id) || !goals.insert(goal.id).second ||
        goal.priority < -1'000'000 || goal.priority > 1'000'000 ||
        goal.activation.size() > 256)
      add("SPRITE_RUNTIME_GOAL_INVALID",
          "runtime goal is malformed or duplicate");
    for (const auto &condition : goal.activation)
      if (!stable_id(condition.variable))
        add("SPRITE_RUNTIME_CONDITION_INVALID",
            "goal condition variable is invalid");
  }
  std::set<std::string> actions;
  for (const auto &value : program.actions) {
    if (!stable_id(value.id) || !actions.insert(value.id).second ||
        !goals.contains(value.goal_id) || value.duration_ticks == 0 ||
        value.duration_ticks > 3'600'000 || value.cooldown_ticks > 86'400'000 ||
        value.energy_cost > 1'000'000 || value.utility_terms.size() > 256 ||
        value.preconditions.size() > 256 || value.markers.size() > 4096)
      add("SPRITE_RUNTIME_ACTION_INVALID",
          "runtime action is malformed, unbounded, duplicate, or references an "
          "absent goal");
    for (const auto &term : value.utility_terms)
      if (!stable_id(term.variable) || term.weight_per_million < -1'000'000 ||
          term.weight_per_million > 1'000'000)
        add("SPRITE_RUNTIME_UTILITY_INVALID", "utility term is invalid");
    for (const auto &condition : value.preconditions)
      if (!stable_id(condition.variable))
        add("SPRITE_RUNTIME_CONDITION_INVALID",
            "action condition variable is invalid");
    std::set<std::pair<std::uint32_t, std::string>> markers;
    for (const auto &marker : value.markers)
      if (!stable_id(marker.id) || marker.tick_offset >= value.duration_ticks ||
          !markers.emplace(marker.tick_offset, marker.id).second)
        add("SPRITE_RUNTIME_MARKER_INVALID",
            "action marker is invalid or duplicate");
  }
  return result;
}

ValidationResult
validate_living_runtime_state(const LivingRuntimeProgram &program,
                              const LivingRuntimeState &state) {
  ValidationResult result;
  const auto program_validation = validate_living_runtime_program(program);
  result.diagnostics.insert(result.diagnostics.end(),
                            program_validation.diagnostics.begin(),
                            program_validation.diagnostics.end());
  if (!program_validation.ok())
    return result;
  try {
    require_state(program, state);
  } catch (const std::exception &error) {
    result.diagnostics.push_back(
        {"SPRITE_RUNTIME_STATE_INVALID", error.what()});
  }
  return result;
}

void set_runtime_variable(LivingRuntimeState &state, std::string key,
                          std::int32_t value) {
  if (!stable_id(key) ||
      state.variables.size() >= 4096 && !state.variables.contains(key))
    throw std::invalid_argument(
        "runtime variable is invalid or variable bound exceeded");
  state.variables.insert_or_assign(std::move(key), value);
}

void observe(LivingRuntimeState &state, const LivingRuntimeProgram &program,
             PerceptionObservation observation) {
  const auto validation = validate_living_runtime_program(program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
  require_state(program, state);
  if (!stable_id(observation.key) || !stable_id(observation.source_entity) ||
      observation.confidence_per_million > 1'000'000 ||
      observation.lifetime_ticks == 0 ||
      observation.lifetime_ticks > 86'400'000 ||
      observation.observed_tick > state.tick)
    throw std::invalid_argument("perception observation is invalid");
  const auto observed_key = observation.key;
  state.memory.push_back({std::move(observation), state.next_sequence++});
  std::ranges::sort(state.memory, [](const auto &left, const auto &right) {
    return std::tuple{left.observation.observed_tick, left.sequence} <
           std::tuple{right.observation.observed_tick, right.sequence};
  });
  std::set<std::string> evicted;
  while (state.memory.size() > program.maximum_memory_records) {
    evicted.insert(state.memory.front().observation.key);
    state.memory.erase(state.memory.begin());
  }
  for (const auto &key : evicted)
    refresh_perception(state, key);
  refresh_perception(state, observed_key);
}

RuntimeStepResult step_living_runtime(const LivingRuntimeProgram &program,
                                      LivingRuntimeState &state) {
  const auto validation = validate_living_runtime_program(program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
  require_state(program, state);
  RuntimeStepResult result;
  std::set<std::string> expired_keys;
  state.memory.erase(
      std::remove_if(state.memory.begin(), state.memory.end(),
                     [&](const auto &record) {
                       const bool expired =
                           state.tick - record.observation.observed_tick >=
                           record.observation.lifetime_ticks;
                       if (expired)
                         expired_keys.insert(record.observation.key);
                       return expired;
                     }),
      state.memory.end());
  for (const auto &key : expired_keys)
    refresh_perception(state, key);
  for (auto iterator = state.cooldowns.begin();
       iterator != state.cooldowns.end();) {
    if (iterator->second > 0)
      --iterator->second;
    if (iterator->second == 0)
      iterator = state.cooldowns.erase(iterator);
    else
      ++iterator;
  }
  if (state.active_action) {
    const auto *definition = action(program, state.active_action->action_id);
    if (definition == nullptr)
      throw std::logic_error("active runtime action is absent from program");
    for (const auto &marker : definition->markers)
      if (marker.tick_offset == state.active_action->elapsed_ticks)
        result.events.push_back(event(state, RuntimeEventKind::action_marker,
                                      definition->id, marker.id));
    ++state.active_action->elapsed_ticks;
    if (state.active_action->remaining_ticks == 0)
      throw std::logic_error(
          "active runtime action has invalid remaining duration");
    if (--state.active_action->remaining_ticks == 0) {
      result.events.push_back(
          event(state, RuntimeEventKind::action_completed, definition->id));
      state.active_action.reset();
    }
  }
  if (!state.active_action) {
    const RuntimeActionDefinition *selected = nullptr;
    std::int64_t selected_score = std::numeric_limits<std::int64_t>::min();
    for (const auto &candidate : program.actions) {
      const auto goal = std::ranges::find(program.goals, candidate.goal_id,
                                          &RuntimeGoalDefinition::id);
      if (goal == program.goals.end() || !conditions(goal->activation, state) ||
          !conditions(candidate.preconditions, state) ||
          candidate.energy_cost > state.energy ||
          state.cooldowns.contains(candidate.id))
        continue;
      std::int64_t score =
          static_cast<std::int64_t>(candidate.base_utility) + goal->priority;
      for (const auto &term : candidate.utility_terms)
        score += static_cast<std::int64_t>(variable(state, term.variable)) *
                 term.weight_per_million / 1'000'000LL;
      if (selected == nullptr || score > selected_score ||
          (score == selected_score && candidate.id < selected->id)) {
        selected = &candidate;
        selected_score = score;
      }
    }
    if (selected) {
      state.energy -= selected->energy_cost;
      if (selected->cooldown_ticks > 0)
        state.cooldowns[selected->id] = selected->cooldown_ticks;
      state.active_action = ActiveRuntimeAction{selected->id, state.tick, 0,
                                                selected->duration_ticks};
      result.selected_action = selected->id;
      result.events.push_back(
          event(state, RuntimeEventKind::action_started, selected->id));
    }
  }
  if (state.tick == std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("living runtime tick overflow");
  ++state.tick;
  return result;
}

bool interrupt_living_action(const LivingRuntimeProgram &program,
                             LivingRuntimeState &state,
                             std::vector<RuntimeEvent> &events) {
  const auto validation = validate_living_runtime_program(program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
  require_state(program, state);
  if (!state.active_action)
    return false;
  const auto *definition = action(program, state.active_action->action_id);
  if (definition == nullptr)
    throw std::logic_error("active runtime action is absent from program");
  if (!definition->interruptible)
    return false;
  events.push_back(
      event(state, RuntimeEventKind::action_interrupted, definition->id));
  state.active_action.reset();
  return true;
}
} // namespace gspl::sprites
