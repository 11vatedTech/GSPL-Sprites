#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/living_runtime.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace gspl::sprites {

/* ═══ CONTROL AUTHORITY ═══
 *
 * One canonical entity + one control policy. The entity carries semantic and
 * gameplay state; the controller produces intents/commands that gameplay
 * systems execute; visual systems manifest the resulting authoritative state.
 * See docs/CONTROL_AUTHORITY.md.
 *
 * This vertical defines the CONTRACT only. Individual policies (player, AI,
 * scripted, network, cinematic, hybrid) are future work; the existing Living
 * runtime program is proven here to be one such policy rather than the
 * definition of the entity.
 */

enum class ControlAuthority {
  player,
  living,
  ai,
  scripted,
  network,
  cinematic,
  hybrid
};

struct ControlPolicy {
  ControlAuthority authority{ControlAuthority::living};
  std::string policy_id;            // stable identity of the controller policy
  std::string entity_instance_id;   // binds the policy to an entity instance
  std::uint32_t priority{};         // execution priority among competing policies
  bool deterministic{true};         // must remain deterministic for replication
};

[[nodiscard]] std::string_view control_authority_name(ControlAuthority authority) noexcept;

/* Minimal validation: policy identity + entity binding must be present. */
[[nodiscard]] ValidationResult validate_control_policy(const ControlPolicy& policy);

/* Deterministic canonical serialization of the policy descriptor. */
[[nodiscard]] std::string canonicalize_control_policy(const ControlPolicy& policy);

/* Prove the existing Living runtime is one control policy: derive the policy
 * descriptor from a LivingRuntimeProgram. The entity itself is untouched. */
[[nodiscard]] ControlPolicy
living_control_policy(const LivingRuntimeProgram& program,
                      std::string_view entity_instance_id);

} // namespace gspl::sprites
