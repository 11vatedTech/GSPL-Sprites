#pragma once

#include "gspl_sprites/runtime_event_router.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gspl::sprites {

struct RuntimeEmission {
  std::uint64_t command_sequence{};
  std::uint64_t source_event_sequence{};
  std::uint64_t tick{};
  std::string operation_id;
  std::int32_t value{};
};

struct RuntimePreviewLimits {
  std::uint32_t maximum_commands_per_batch{65'536};
  std::uint32_t maximum_pending_combat{4'096};
  std::uint32_t maximum_pending_effects{4'096};
  std::uint32_t maximum_pending_audio{4'096};
};

struct RuntimePreviewState {
  std::uint64_t next_command_sequence{};
  std::uint64_t last_tick{};
  std::optional<std::string> animation_operation;
  std::uint64_t animation_started_tick{};
  std::vector<RuntimeEmission> combat;
  std::vector<RuntimeEmission> effects;
  std::vector<RuntimeEmission> audio;
};

void apply_runtime_consumer_commands(
    RuntimePreviewState &state,
    std::span<const RuntimeConsumerCommand> commands,
    const RuntimePreviewLimits &limits = {});

} // namespace gspl::sprites
