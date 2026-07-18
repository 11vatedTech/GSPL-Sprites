#pragma once

#include "gspl_sprites/runtime_persistence.hpp"

namespace gspl::sprites {

struct RuntimeReplicationUpdate {
  std::string program_identity;
  std::string base_state_identity;
  std::string target_state_identity;
  std::uint64_t target_tick{};
  std::string target_state;
};

[[nodiscard]] RuntimeReplicationUpdate make_runtime_replication_update(
    const LivingRuntimeProgram &program, const LivingRuntimeState &base,
    const LivingRuntimeState &authoritative);
[[nodiscard]] std::string serialize_runtime_replication_update(
    const RuntimeReplicationUpdate &update);
[[nodiscard]] RuntimeReplicationUpdate deserialize_runtime_replication_update(
    std::string_view source,
    std::uint64_t maximum_bytes = 16ULL * 1024ULL * 1024ULL);
void apply_runtime_replication_update(
    const LivingRuntimeProgram &program, LivingRuntimeState &local_state,
    const RuntimeReplicationUpdate &update);

} // namespace gspl::sprites
