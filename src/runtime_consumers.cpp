#include "gspl_sprites/runtime_consumers.hpp"

#include <limits>
#include <stdexcept>

namespace gspl::sprites {
namespace {
void require_limits(const RuntimePreviewLimits &limits) {
  if (limits.maximum_commands_per_batch == 0 ||
      limits.maximum_pending_combat == 0 ||
      limits.maximum_pending_effects == 0 || limits.maximum_pending_audio == 0)
    throw std::invalid_argument("runtime preview limits must be positive");
}
void append(std::vector<RuntimeEmission> &target, std::uint32_t maximum,
            const RuntimeConsumerCommand &command) {
  if (target.size() >= maximum)
    throw std::runtime_error("runtime preview queue limit exceeded");
  target.push_back({command.command_sequence, command.event_sequence,
                    command.tick, command.operation_id, command.value});
}
} // namespace

void apply_runtime_consumer_commands(
    RuntimePreviewState &state,
    std::span<const RuntimeConsumerCommand> commands,
    const RuntimePreviewLimits &limits) {
  require_limits(limits);
  if (commands.size() > limits.maximum_commands_per_batch)
    throw std::invalid_argument("runtime preview command batch exceeds limit");
  auto candidate = state;
  for (const auto &command : commands) {
    if (command.command_sequence != candidate.next_command_sequence ||
        command.tick < candidate.last_tick)
      throw std::invalid_argument("runtime consumer command order is invalid");
    switch (command.consumer) {
    case RuntimeConsumerKind::animation:
      candidate.animation_operation = command.operation_id;
      candidate.animation_started_tick = command.tick;
      break;
    case RuntimeConsumerKind::combat:
      append(candidate.combat, limits.maximum_pending_combat, command);
      break;
    case RuntimeConsumerKind::effect:
      append(candidate.effects, limits.maximum_pending_effects, command);
      break;
    case RuntimeConsumerKind::audio:
      append(candidate.audio, limits.maximum_pending_audio, command);
      break;
    }
    candidate.last_tick = command.tick;
    if (candidate.next_command_sequence ==
        std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("runtime consumer command sequence overflow");
    ++candidate.next_command_sequence;
  }
  state = std::move(candidate);
}
} // namespace gspl::sprites
