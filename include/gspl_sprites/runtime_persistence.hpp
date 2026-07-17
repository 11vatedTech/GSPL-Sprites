#pragma once

#include "gspl_sprites/living_runtime.hpp"

namespace gspl::sprites {

struct RuntimeVariableUpdate {
  std::string key;
  std::int32_t value{};
};
struct RuntimeReplayFrame {
  std::uint64_t tick{};
  std::vector<RuntimeVariableUpdate> variables;
  std::vector<PerceptionObservation> observations;
  bool interrupt_active_action{};
};
struct RuntimeReplayLimits {
  std::uint64_t maximum_ticks{10'000'000};
  std::uint64_t maximum_events{10'000'000};
  std::uint64_t maximum_frames{1'000'000};
};
struct RuntimeReplayResult {
  LivingRuntimeState final_state;
  std::vector<RuntimeEvent> events;
  std::string final_state_identity;
};

[[nodiscard]] std::string
canonicalize_living_runtime_program(const LivingRuntimeProgram &program);
[[nodiscard]] std::string
living_runtime_program_identity(const LivingRuntimeProgram &program);
[[nodiscard]] std::string
serialize_living_runtime_state(const LivingRuntimeProgram &program,
                               const LivingRuntimeState &state);
[[nodiscard]] LivingRuntimeState deserialize_living_runtime_state(
    const LivingRuntimeProgram &program, std::string_view source,
    std::uint64_t maximum_bytes = 16ULL * 1024ULL * 1024ULL);
[[nodiscard]] RuntimeReplayResult replay_living_runtime(
    const LivingRuntimeProgram &program, LivingRuntimeState initial_state,
    std::span<const RuntimeReplayFrame> frames, std::uint64_t end_tick,
    const RuntimeReplayLimits &limits = {});

} // namespace gspl::sprites
