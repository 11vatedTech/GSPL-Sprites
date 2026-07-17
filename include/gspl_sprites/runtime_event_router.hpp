#pragma once

#include "gspl_sprites/living_runtime.hpp"

#include <span>

namespace gspl::sprites {

enum class RuntimeConsumerKind { animation, combat, effect, audio };

struct RuntimeEventBinding {
  RuntimeEventKind event_kind{};
  std::string action_id;
  std::string marker_id;
  RuntimeConsumerKind consumer{};
  std::string operation_id;
  std::int32_t value{};
};

struct RuntimeEventRoutingProgram {
  std::string id;
  std::vector<RuntimeEventBinding> bindings;
  std::uint32_t maximum_commands_per_event{32};
};

struct RuntimeConsumerCommand {
  std::uint64_t tick{};
  std::uint64_t event_sequence{};
  RuntimeConsumerKind consumer{};
  std::string operation_id;
  std::int32_t value{};
};

struct RuntimeEventRouterState {
  std::uint64_t next_event_sequence{};
};

[[nodiscard]] ValidationResult validate_runtime_event_routing_program(
    const RuntimeEventRoutingProgram &program,
    const LivingRuntimeProgram &runtime_program);
[[nodiscard]] std::string canonicalize_runtime_event_routing_program(
    const RuntimeEventRoutingProgram &program,
    const LivingRuntimeProgram &runtime_program);
[[nodiscard]] std::vector<RuntimeConsumerCommand>
route_runtime_events(const RuntimeEventRoutingProgram &program,
                     const LivingRuntimeProgram &runtime_program,
                     RuntimeEventRouterState &state,
                     std::span<const RuntimeEvent> events,
                     std::uint64_t maximum_commands = 65'536);

} // namespace gspl::sprites
