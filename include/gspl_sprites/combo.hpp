#pragma once

#include "gspl_sprites/combat.hpp"

#include <optional>

namespace gspl::sprites {

struct ComboTransition {
  std::string from_ability;
  std::string to_ability;
  std::uint32_t earliest_tick_offset{};
  std::uint32_t latest_tick_offset{};
  bool requires_hit_confirmation{};
};
struct ComboProgram {
  std::string id;
  std::uint32_t maximum_history{64};
  std::vector<std::string> entry_abilities;
  std::vector<ComboTransition> transitions;
};
struct ComboHistoryEntry {
  std::string ability_id;
  std::uint64_t started_tick{};
  bool hit_confirmed{};
};
struct ComboState {
  std::optional<std::string> active_ability;
  std::uint64_t active_started_tick{};
  bool active_hit_confirmed{};
  std::vector<ComboHistoryEntry> history;
};
struct ComboCombatResult {
  std::vector<CombatEvent> combat_events;
  bool entered_combo{};
  bool transitioned{};
};

[[nodiscard]] ValidationResult
validate_combo_program(const ComboProgram &program,
                       const CombatProgram &combat_program);
[[nodiscard]] ValidationResult
validate_combo_state(const ComboProgram &program,
                     const CombatProgram &combat_program,
                     const ComboState &state);
[[nodiscard]] ComboCombatResult execute_combo_combat_command(
    const ComboProgram &program, const CombatProgram &combat_program,
    ComboState &combo_state, CombatState &combat_state,
    const CombatCommand &command, bool confirms_previous_hit = false);
void end_combo(const ComboProgram &program,
               const CombatProgram &combat_program, ComboState &state);

} // namespace gspl::sprites
