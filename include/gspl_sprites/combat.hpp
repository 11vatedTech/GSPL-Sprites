#pragma once

#include "gspl_sprites/common.hpp"

#include <map>

namespace gspl::sprites {

enum class CombatTargetRule { self, ally, enemy, any };
enum class CombatEffectKind { damage, healing, status };

struct CombatEffectDefinition {
  CombatEffectKind kind{};
  std::string status_id;
  std::uint32_t magnitude{};
  std::uint32_t duration_ticks{};
};
struct CombatAbilityDefinition {
  std::string id;
  CombatTargetRule target_rule{CombatTargetRule::enemy};
  std::uint32_t resource_cost{};
  std::uint32_t cooldown_ticks{};
  std::uint32_t maximum_range_millimeters{};
  std::vector<CombatEffectDefinition> effects;
};
struct CombatProgram {
  std::string id;
  std::uint32_t maximum_actors{1024};
  std::uint32_t maximum_statuses_per_actor{64};
  std::vector<CombatAbilityDefinition> abilities;
};
struct CombatStatusState {
  std::uint32_t magnitude{};
  std::uint64_t expires_tick{};
};
struct CombatActorState {
  std::string id;
  std::string team_id;
  std::uint32_t health{};
  std::uint32_t maximum_health{};
  std::uint32_t resource{};
  std::uint32_t maximum_resource{};
  std::int32_t x_millimeters{};
  std::int32_t y_millimeters{};
  std::map<std::string, std::uint64_t, std::less<>> cooldown_expiry;
  std::map<std::string, CombatStatusState, std::less<>> statuses;
};
struct CombatState {
  std::uint64_t tick{};
  std::uint64_t next_sequence{};
  std::map<std::string, CombatActorState, std::less<>> actors;
};
struct CombatCommand {
  std::uint64_t tick{};
  std::string source_actor;
  std::string target_actor;
  std::string ability_id;
};
struct CombatEvent {
  std::uint64_t tick{};
  std::uint64_t sequence{};
  std::string source_actor;
  std::string target_actor;
  std::string ability_id;
  CombatEffectKind effect{};
  std::string status_id;
  std::uint32_t applied_magnitude{};
};

[[nodiscard]] ValidationResult validate_combat_program(const CombatProgram &program);
[[nodiscard]] std::string canonicalize_combat_program(const CombatProgram &program);
[[nodiscard]] std::string combat_program_identity(const CombatProgram &program);
[[nodiscard]] ValidationResult validate_combat_state(const CombatProgram &program,
                                                     const CombatState &state);
[[nodiscard]] std::vector<CombatEvent>
execute_combat_command(const CombatProgram &program, CombatState &state,
                       const CombatCommand &command);
void advance_combat_to(const CombatProgram &program, CombatState &state,
                       std::uint64_t tick);

} // namespace gspl::sprites
