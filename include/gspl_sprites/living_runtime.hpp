#pragma once

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace gspl::sprites {

enum class RuntimeEventKind {
  action_started,
  action_marker,
  action_completed,
  action_interrupted,
  diagnostic
};

struct RuntimeCondition {
  std::string variable;
  Comparison comparison{Comparison::equal};
  std::int32_t threshold{};
};
struct UtilityTerm {
  std::string variable;
  std::int32_t weight_per_million{};
};
struct ActionMarker {
  std::string id;
  std::uint32_t tick_offset{};
};

struct RuntimeGoalDefinition {
  std::string id;
  std::int32_t priority{};
  std::vector<RuntimeCondition> activation;
};

struct RuntimeActionDefinition {
  std::string id;
  std::string goal_id;
  std::int32_t base_utility{};
  std::vector<UtilityTerm> utility_terms;
  std::vector<RuntimeCondition> preconditions;
  std::uint32_t duration_ticks{1};
  std::uint32_t cooldown_ticks{};
  std::uint32_t energy_cost{};
  bool interruptible{};
  std::vector<ActionMarker> markers;
};

struct LivingRuntimeProgram {
  std::string id;
  std::uint32_t ticks_per_second{60};
  std::uint32_t maximum_memory_records{1024};
  std::vector<RuntimeGoalDefinition> goals;
  std::vector<RuntimeActionDefinition> actions;
};

struct PerceptionObservation {
  std::string key;
  std::string source_entity;
  std::int32_t value{};
  std::uint32_t confidence_per_million{};
  std::uint64_t observed_tick{};
  std::uint32_t lifetime_ticks{1};
};

struct RuntimeMemoryRecord {
  PerceptionObservation observation;
  std::uint64_t sequence{};
};

struct ActiveRuntimeAction {
  std::string action_id;
  std::uint64_t started_tick{};
  std::uint32_t elapsed_ticks{};
  std::uint32_t remaining_ticks{};
};

struct EntityStateIdentity {
  std::string entity_def_id;
  std::string instance_id;
  std::string current_form;
  std::string active_transformation;
  std::uint64_t transformation_start_tick{};
  std::uint64_t transformation_completion_tick{};
  double transformation_progress{};
  std::uint64_t runtime_tick{};
  std::uint32_t health{100};
  std::uint32_t resource{100};
  std::map<std::string, std::uint32_t, std::less<>> cooldowns;
  std::map<std::string, std::uint32_t, std::less<>> statuses;
  std::string active_ability;
  std::string projectile_state;
  std::string combo_state;
  std::string behavior_state;
  std::string deterministic_event_hash;
};

struct LivingRuntimeState {
  std::uint64_t tick{};
  std::uint64_t next_sequence{};
  std::uint32_t energy{100};
  std::map<std::string, std::int32_t, std::less<>> variables;
  std::vector<RuntimeMemoryRecord> memory;
  std::map<std::string, std::uint32_t, std::less<>> cooldowns;
  std::optional<ActiveRuntimeAction> active_action;
};

struct RuntimeEvent {
  std::uint64_t tick{};
  std::uint64_t sequence{};
  RuntimeEventKind kind{};
  std::string action_id;
  std::string marker_id;
};

struct RuntimeStepResult {
  std::vector<RuntimeEvent> events;
  std::optional<std::string> selected_action;
};

[[nodiscard]] ValidationResult
validate_living_runtime_program(const LivingRuntimeProgram &program);
[[nodiscard]] ValidationResult
validate_living_runtime_state(const LivingRuntimeProgram &program,
                              const LivingRuntimeState &state);
void set_runtime_variable(LivingRuntimeState &state, std::string key,
                          std::int32_t value);
void set_health_percent(LivingRuntimeState &state, std::uint32_t health,
                        std::uint32_t maximum_health);
void observe(LivingRuntimeState &state, const LivingRuntimeProgram &program,
             PerceptionObservation observation);
[[nodiscard]] RuntimeStepResult
step_living_runtime(const LivingRuntimeProgram &program,
                    LivingRuntimeState &state);
[[nodiscard]] bool interrupt_living_action(const LivingRuntimeProgram &program,
                                           LivingRuntimeState &state,
                                           std::vector<RuntimeEvent> &events);
[[nodiscard]] EntityStateIdentity
capture_entity_identity(const LivingRuntimeState &state,
                        std::string_view entity_def_id,
                        std::string_view instance_id,
                        std::string_view current_form,
                        std::string_view active_transformation,
                        std::string_view active_ability,
                        std::string_view projectile_state,
                        std::string_view combo_state,
                        std::string_view behavior_state);
[[nodiscard]] EntityStateIdentity
capture_entity_identity(const LivingRuntimeState &state,
                        const LivingRuntimeState &combat_state,
                        const LivingRuntimeState &transformation_state,
                        std::string_view entity_def_id,
                        std::string_view instance_id);
[[nodiscard]] std::string
entity_identity_hash(const EntityStateIdentity &identity);
[[nodiscard]] bool
entity_identities_match(const EntityStateIdentity &a,
                        const EntityStateIdentity &b);

} // namespace gspl::sprites
