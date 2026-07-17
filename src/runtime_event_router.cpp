#include "gspl_sprites/runtime_event_router.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <tuple>

namespace gspl::sprites {
namespace {
bool stable_id(std::string_view value) {
  if (value.empty() || value.size() > 128)
    return false;
  return std::ranges::all_of(value, [](unsigned char c) {
    return std::isalnum(c) || c == '.' || c == '_' || c == '-';
  });
}
void diagnostic(ValidationResult &result, std::string code,
                std::string message) {
  result.diagnostics.push_back({std::move(code), std::move(message)});
}
auto binding_key(const RuntimeEventBinding &value) {
  return std::tuple{value.event_kind, value.action_id,    value.marker_id,
                    value.consumer,   value.operation_id, value.value};
}
bool matches(const RuntimeEventBinding &binding, const RuntimeEvent &event) {
  return binding.event_kind == event.kind &&
         binding.action_id == event.action_id &&
         binding.marker_id == event.marker_id;
}
bool operation_matches_consumer(const RuntimeEventBinding &binding) {
  switch (binding.consumer) {
  case RuntimeConsumerKind::animation:
    return binding.operation_id.starts_with("animation.");
  case RuntimeConsumerKind::combat:
    return binding.operation_id.starts_with("ability.");
  case RuntimeConsumerKind::effect:
    return binding.operation_id.starts_with("effect.");
  case RuntimeConsumerKind::audio:
    return binding.operation_id.starts_with("audio.");
  }
  return false;
}
} // namespace

ValidationResult validate_runtime_event_routing_program(
    const RuntimeEventRoutingProgram &program,
    const LivingRuntimeProgram &runtime_program) {
  ValidationResult result;
  const auto runtime_validation =
      validate_living_runtime_program(runtime_program);
  result.diagnostics.insert(result.diagnostics.end(),
                            runtime_validation.diagnostics.begin(),
                            runtime_validation.diagnostics.end());
  if (!stable_id(program.id))
    diagnostic(result, "SPRITE_ROUTER_ID_INVALID",
               "routing program ID is invalid");
  if (program.bindings.size() > 16'384 ||
      program.maximum_commands_per_event == 0 ||
      program.maximum_commands_per_event > 1'024)
    diagnostic(result, "SPRITE_ROUTER_LIMIT_INVALID",
               "routing program limits are invalid");
  std::set<std::tuple<RuntimeEventKind, std::string, std::string,
                      RuntimeConsumerKind, std::string>>
      unique;
  for (const auto &binding : program.bindings) {
    const auto action =
        std::ranges::find(runtime_program.actions, binding.action_id,
                          &RuntimeActionDefinition::id);
    if (action == runtime_program.actions.end() ||
        !stable_id(binding.operation_id) ||
        !operation_matches_consumer(binding) ||
        binding.event_kind == RuntimeEventKind::diagnostic)
      diagnostic(result, "SPRITE_ROUTER_BINDING_INVALID",
                 "routing binding references invalid semantics");
    if ((binding.event_kind == RuntimeEventKind::action_marker) !=
        !binding.marker_id.empty())
      diagnostic(result, "SPRITE_ROUTER_MARKER_INVALID",
                 "only marker events may name a marker");
    if (action != runtime_program.actions.end() && !binding.marker_id.empty() &&
        std::ranges::find(action->markers, binding.marker_id,
                          &ActionMarker::id) == action->markers.end())
      diagnostic(result, "SPRITE_ROUTER_MARKER_UNKNOWN",
                 "routing binding references an unknown marker");
    if (!unique
             .emplace(binding.event_kind, binding.action_id, binding.marker_id,
                      binding.consumer, binding.operation_id)
             .second)
      diagnostic(result, "SPRITE_ROUTER_BINDING_DUPLICATE",
                 "routing binding is duplicate");
  }
  return result;
}

std::string canonicalize_runtime_event_routing_program(
    const RuntimeEventRoutingProgram &program,
    const LivingRuntimeProgram &runtime_program) {
  const auto validation =
      validate_runtime_event_routing_program(program, runtime_program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto bindings = program.bindings;
  std::ranges::sort(bindings, {}, binding_key);
  std::ostringstream output;
  output << "schema=gspl.runtime-event-routing/0.1\nid=" << program.id
         << "\nmaximum_commands_per_event="
         << program.maximum_commands_per_event << '\n';
  for (const auto &binding : bindings)
    output << "binding=" << static_cast<int>(binding.event_kind) << '|'
           << binding.action_id << '|' << binding.marker_id << '|'
           << static_cast<int>(binding.consumer) << '|' << binding.operation_id
           << '|' << binding.value << '\n';
  return output.str();
}

std::vector<RuntimeConsumerCommand> route_runtime_events(
    const RuntimeEventRoutingProgram &program,
    const LivingRuntimeProgram &runtime_program, RuntimeEventRouterState &state,
    std::span<const RuntimeEvent> events, std::uint64_t maximum_commands) {
  const auto validation =
      validate_runtime_event_routing_program(program, runtime_program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  if (maximum_commands == 0)
    throw std::invalid_argument("runtime command limit must be positive");
  auto bindings = program.bindings;
  std::ranges::sort(bindings, {}, binding_key);
  std::vector<RuntimeConsumerCommand> commands;
  auto next_event_sequence = state.next_event_sequence;
  auto next_command_sequence = state.next_command_sequence;
  for (const auto &event : events) {
    if (event.sequence != next_event_sequence)
      throw std::invalid_argument("runtime event sequence is not contiguous");
    std::uint32_t emitted{};
    for (const auto &binding : bindings)
      if (matches(binding, event)) {
        if (++emitted > program.maximum_commands_per_event ||
            commands.size() >= maximum_commands)
          throw std::runtime_error("runtime event command limit exceeded");
        if (next_command_sequence == std::numeric_limits<std::uint64_t>::max())
          throw std::overflow_error("runtime command sequence overflow");
        commands.push_back({next_command_sequence++, event.tick, event.sequence,
                            binding.consumer, binding.operation_id,
                            binding.value});
      }
    if (next_event_sequence == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("runtime event sequence overflow");
    ++next_event_sequence;
  }
  state.next_event_sequence = next_event_sequence;
  state.next_command_sequence = next_command_sequence;
  return commands;
}
} // namespace gspl::sprites
