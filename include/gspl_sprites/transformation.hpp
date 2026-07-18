#pragma once

#include "gspl_sprites/combat.hpp"

#include <optional>

namespace gspl::sprites {

struct TransformationFormDefinition {
  std::string id;
  std::int32_t maximum_health_delta{};
  std::int32_t maximum_resource_delta{};
  std::vector<std::string> enabled_abilities;
};
struct TransformationDefinition {
  std::string id;
  std::string from_form;
  std::string to_form;
  std::uint32_t energy_cost{};
  std::uint32_t duration_ticks{1};
  bool reversible{};
};
struct TransformationProgram {
  std::string id;
  std::string base_form;
  std::uint32_t maximum_history{64};
  std::vector<TransformationFormDefinition> forms;
  std::vector<TransformationDefinition> transformations;
};
struct ActiveTransformation {
  std::string transformation_id;
  std::string from_form;
  std::string to_form;
  std::uint64_t started_tick{};
  std::uint64_t completes_tick{};
};
struct TransformationHistoryEntry {
  std::string transformation_id;
  std::string from_form;
  std::string to_form;
  std::uint64_t completed_tick{};
};
struct TransformationState {
  std::string stable_entity_id;
  std::string current_form;
  std::uint32_t energy{};
  std::uint32_t maximum_energy{};
  std::optional<ActiveTransformation> active;
  std::vector<TransformationHistoryEntry> history;
};

[[nodiscard]] ValidationResult validate_transformation_program(
    const TransformationProgram &program, const CombatProgram &combat_program);
[[nodiscard]] ValidationResult validate_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state);
void begin_transformation(const TransformationProgram &program,
                          const CombatProgram &combat_program,
                          TransformationState &state,
                          std::string_view transformation_id,
                          std::uint64_t tick);
void advance_transformation_to(const TransformationProgram &program,
                               const CombatProgram &combat_program,
                               TransformationState &state,
                               CombatState &combat_state, std::uint64_t tick);
[[nodiscard]] std::vector<CombatEvent> execute_transformed_combat_command(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &transformation_state, CombatState &combat_state,
    const CombatCommand &command);

} // namespace gspl::sprites
