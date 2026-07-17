#include "gspl_sprites/living_runtime.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
LivingRuntimeProgram program() {
  return {
      "voltfox.runtime",
      60,
      2,
      {{"defend", 100, {{"perception.threat", Comparison::greater_equal, 1}}},
       {"idle", 0, {}}},
      {{"attack",
        "defend",
        10,
        {{"perception.threat", 500000}},
        {{"confidence.threat", Comparison::greater_equal, 500000}},
        2,
        3,
        20,
        true,
        {{"release", 0}}},
       {"observe", "idle", 1, {}, {}, 1, 0, 0, true, {}}}};
}
} // namespace

int main() {
  try {
    const auto definition = program();
    check(validate_living_runtime_program(definition).ok(),
          "valid living runtime rejected");
    LivingRuntimeState state;
    auto first = step_living_runtime(definition, state);
    check(first.selected_action == "observe" && state.tick == 1 &&
              state.active_action->action_id == "observe",
          "idle behavior selection failed");
    auto second = step_living_runtime(definition, state);
    check(second.events.size() == 2 &&
              second.events[0].kind == RuntimeEventKind::action_completed &&
              second.selected_action == "observe",
          "action completion or deterministic reselection failed");
    std::vector<RuntimeEvent> interrupted;
    check(interrupt_living_action(definition, state, interrupted) &&
              interrupted.size() == 1 &&
              interrupted[0].kind == RuntimeEventKind::action_interrupted,
          "interruptible action did not stop");
    observe(state, definition, {"threat", "enemy.1", 2, 900000, state.tick, 2});
    auto attack = step_living_runtime(definition, state);
    check(attack.selected_action == "attack" && state.energy == 80,
          "perception-driven utility action not selected");
    auto executing = step_living_runtime(definition, state);
    check(executing.events.size() == 1 &&
              executing.events[0].kind == RuntimeEventKind::action_marker &&
              executing.events[0].marker_id == "release",
          "action marker did not fire deterministically");
    std::vector<RuntimeEvent> attack_interruption;
    check(interrupt_living_action(definition, state, attack_interruption),
          "attack interruption failed");
    auto cooldown = step_living_runtime(definition, state);
    check(cooldown.selected_action == "observe", "cooldown admission failed");
    observe(state, definition, {"threat", "enemy.2", 3, 800000, state.tick, 1});
    observe(state, definition, {"sound", "enemy.3", 1, 700000, state.tick, 1});
    observe(state, definition, {"light", "world", 1, 600000, state.tick, 1});
    check(state.memory.size() == 2, "bounded memory eviction failed");
    (void)step_living_runtime(definition, state);
    (void)step_living_runtime(definition, state);
    check(state.memory.empty() &&
              !state.variables.contains("perception.light") &&
              !state.variables.contains("confidence.light"),
          "expired perceptions remained in memory or blackboard");
    auto invalid = definition;
    invalid.actions[0].goal_id = "absent";
    check(!validate_living_runtime_program(invalid).ok(),
          "action with absent goal accepted");
    auto duplicate = definition;
    duplicate.actions.push_back(duplicate.actions[0]);
    check(!validate_living_runtime_program(duplicate).ok(),
          "duplicate action accepted");
    auto invalid_state = state;
    invalid_state.energy = 1'000'001;
    bool state_rejected = false;
    try {
      (void)step_living_runtime(definition, invalid_state);
    } catch (const std::invalid_argument &) {
      state_rejected = true;
    }
    check(state_rejected, "invalid runtime snapshot accepted");
    std::cout << "all gspl sprites living runtime tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
